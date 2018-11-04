open Phx;
open MessageMap;

let subtract = (a1, a2) => {
  let l2 = Array.to_list(a2);
  Array.to_list(a1)
  |> List.filter(e => !List.mem(e, l2))
  |> Array.of_list;
};

type ready_state = {
  id: string,
  name: string,
  socket: Socket.t,
  channel: Channel.t,
  rooms: Room.by_room_id,
  available: array(string), /* redundant? */
  entered: array(string),
  messages: Messages.by_room_id_t,
  text: string,
  selected: option(string)
};

type state =
  | Connecting
  | Ready(ready_state)
  | Error;

type action =
  | Connect
  | Connected(string, string, Socket.t, Channel.t)
  | RoomCreate
  | RoomCreated(string, string, string)
  | RoomEnter(string)
  | RoomSelect(string)
  | ReceiveRoomSetting(string, string, string)
  | SendRoomSetting(option(string), option(string))
  | ReceiveAvatarProfile(string, string)
  | SendAvatarProfile(string)
  | Send
  | Receive(string, string, string)
  | UpdateText(string);

let component = ReasonReact.reducerComponent("App");

let make = _children => {
  ...component,
  initialState: () => Connecting,
  didMount: self => {
    self.send(Connect);
  },
  reducer: (action, state) =>
    switch (action) {
    | Connect =>
      ReasonReact.SideEffects(self => {
        let socket = initSocket("/socket")
                     |> connectSocket;
        let channel = socket
                      |> initChannel("lounge:hello");
        let _ =
          channel
          |> putOn("room:message", (res: Abstract.any) => {
            let { source, room_id, body }: Decode.message = Decode.receive(res);
            self.send(Receive(source, room_id, body));
          })
          |> putOn("room:setting", (res: Abstract.any) => {
            let { room_id, name, color }: Decode.setting = Decode.setting(res);
            self.send(ReceiveRoomSetting(room_id, name, color));
          })
          |> putOn("avatar:profile", (res: Abstract.any) => {
            let { avatar_id, name }: Decode.profile = Decode.profile(res);
            self.send(ReceiveAvatarProfile(avatar_id, name));
          })
          |> joinChannel
          |> putReceive("ok", (res: Abstract.any) => {
            let welcome: Decode.welcome = Decode.welcome(res);
            self.send(Connected(welcome.id, welcome.name, socket, channel));
          });
      })
    | Connected(id, name, socket, channel) =>
      ReasonReact.Update(Ready({
        id,
        name,
        socket,
        channel,
        rooms: Room.RoomMap.empty,
        available: [||],
        entered: [||],
        messages: MessageMap.empty,
        text: "",
        selected: None
      }))
    | Send => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel, text, selected }) =>
        switch (selected) {
        | Some(room) =>
          push("room:message", {"source": id, "room_id": room, "body": text}, channel) |> ignore;
          /* Loopback. Update self state */
          self.send(Receive(id, room, text));
          self.send(UpdateText(""));
        | None => ()
        }
      | _ => ()
      }})
    | Receive(source, room_id, body) =>
      switch (state) {
      | Ready({ messages } as state) =>
        ReasonReact.Update(Ready({
          ...state,
          messages: Message.addMsg(source, room_id, body, messages)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomCreate => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel }) =>
        push("room:create", {"source": id}, channel)
          |> putReceive("ok", (res: Abstract.any) => {
            let { room_id, name, color }: Decode.setting = Decode.setting(res);
            self.send(RoomCreated(room_id, name, color));
          })
          |> ignore;
        ();
      | _ => ()
      }})
    | RoomCreated(room_id, name, color) =>
      switch (state) {
      | Ready({ rooms, available, entered } as state) =>
        ReasonReact.Update(Ready({
          ...state,
          rooms: Room.upsertRoom(room_id, name, color, rooms),
          available: Room.uniqRooms(room_id, available),
          entered: Room.uniqRooms(room_id, entered),
          selected: Some(room_id)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomEnter(room_id) =>
      switch (state) {
      | Ready({ id, channel, entered } as state) =>
        push("room:enter", {"source": id, "room_id": room_id}, channel) |> ignore;
        ReasonReact.Update(Ready({
          ...state,
          entered: Room.uniqRooms(room_id, entered),
          selected: Some(room_id)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomSelect(room_id) =>
      switch (state) {
      | Ready({ selected } as state) =>
        ReasonReact.Update(Ready({
          ...state,
          selected: Some(room_id)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | ReceiveRoomSetting(room_id, name, color) =>
      switch (state) {
      | Ready({ rooms, available } as state) =>
        ReasonReact.Update(Ready({
          ...state,
          rooms: Room.upsertRoom(room_id, name, color, rooms),
          available: Room.uniqRooms(room_id, available)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | SendRoomSetting(name_opt, color_opt) => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel, rooms, selected }) =>
        switch (selected) {
        | Some(room) =>
          let name = switch (name_opt) {
          | Some(name) => name
          | None => Room.getRoomName(room, rooms)
          };
          let color = switch (color_opt) {
          | Some(color) => color
          | None => Room.getRoomColor(room, rooms)
          };
          push("room:setting", {"source": id, "room_id": room, "name": name, "color": color}, channel) |> ignore;
          /* Loopback. Update self state */
          self.send(ReceiveRoomSetting(room, name, color));
        | None => ()
        }
      | _ => ()
      }})
    | ReceiveAvatarProfile(avatar_id, name) =>
      switch (state) {
      | Ready(state) => ReasonReact.Update(Ready({...state, name}))
      | _ => ReasonReact.NoUpdate
      }
    | SendAvatarProfile(name) => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel }) =>
        push("avatar:profile", {"source": id, "name": name}, channel) |> ignore;
        /* Loopback. Update self state */
        self.send(ReceiveAvatarProfile(id, name));
      | _ => ()
      }})
    | UpdateText(input) =>
      switch (state) {
      | Ready({ text } as state) =>
        ReasonReact.Update(Ready({
          ...state,
          text: input
        }))
      | _ => ReasonReact.NoUpdate
      }
    },
  render: self => {
    <div className="p-container">
      (switch (self.state) {
      | Ready({ name, rooms, available, entered, messages, text, selected }) =>
        <>
          <div className=(Room.roomClassName(selected, rooms))>
            <header className="c-header">{ReasonReact.string("CizenChat")}</header>
            <div className="p-side-content">
              <InPlaceEdit
                name="user"
                text=name
                handleChange=(value => SendAvatarProfile(value) |> self.send)
              />

              <button className="c-button" onClick=(_event => RoomCreate |> self.send)>
                (ReasonReact.string("Create Room"))
              </button>

              <RoomList
                title="Available Rooms"
                rooms=(Room.byIds(subtract(available, entered), rooms))
                handleSelect=(room => RoomEnter(room) |> self.send)
              />

              <RoomList
                title="Joined Rooms"
                rooms=(Room.byIds(entered, rooms))
                handleSelect=(room => RoomSelect(room) |> self.send)
              />
            </div>
          </div>
          <div className="p-chat">
            <div className="c-chat">
              (switch (selected) {
              | Some(room) =>
                <>
                  <div className="c-chat-header">
                    <InPlaceEdit
                      name="room-title"
                      text=(Room.getRoomName(room, rooms))
                      handleChange=(value => SendRoomSetting(Some(value), None) |> self.send)
                    />
                    <ThemeChanger
                      handleChange=(color => SendRoomSetting(None, Some(color)) |> self.send)
                    />
                  </div>
                  <MessageList messages=(Message.getMsg(room, messages)) />
                </>
              | None => <p>(ReasonReact.string("Select or create a room"))</p>
              })
            </div>
            <div className="c-text-area-wrapper">
              <div className="c-text-area">
                <textarea 
                  rows=1
                  placeholder="What's up?"
                  value=text
                  onKeyDown=(
                    event =>
                      if (ReactEvent.Keyboard.keyCode(event) === 13) {
                        ReactEvent.Keyboard.preventDefault(event);
                        self.send(Send);
                      }
                  )
                  onChange=(
                    event =>
                      self.send(UpdateText(ReactEvent.Form.target(event)##value))
                  )
                >
                </textarea>
              </div>
            </div>
          </div>
          <div className="p-avatars">
          </div>
        </>
      | _ => <div>(ReasonReact.string("Connecting..."))</div>
      })
    </div>;
  },
};

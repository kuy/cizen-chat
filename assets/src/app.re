open Phx;

/* type: message */

module MsgMap = Map.Make({
  type t = string;
  let compare = compare;
});

type message = {
  body: string,
  avatar_id: string
};
type messages = array(message);
type messages_by_room_id = MsgMap.t(messages);

let addMsg = (avatar_id, room, body, map) => {
  let msg = { body, avatar_id };
  if (MsgMap.mem(room, map)) {
    let messages = MsgMap.find(room, map);
    MsgMap.add(room, Array.concat([messages, [|msg|]]), map);
  } else {
    MsgMap.add(room, [|msg|], map);
  };
};

let getMsg = (room, map) =>
  try(MsgMap.find(room, map)) {
  | Not_found => [||]
  };

/* type: room */

module RoomMap = Map.Make({
  type t = string;
  let compare = compare;
});

type room = {
  id: string,
  color: string
};
type room_by_room_id = RoomMap.t(room);

let uniqRooms = (room_id, rooms) => {
  let rooms_l = Array.to_list(rooms);
  if (List.mem(room_id, rooms_l)) {
    rooms;
  } else {
    Array.concat([rooms, [|room_id|]]);
  };
};

let upsertRoom = (room_id, color, rooms) => {
  let room = { id: room_id, color };
  RoomMap.add(room_id, room, rooms);
};

let roomClassName = (room_id_opt, rooms) => {
  let color = switch (room_id_opt) {
  | Some(room_id) =>
    try(RoomMap.find(room_id, rooms).color) {
    | Not_found => "green"
    }
  | None => "green"
  };
  "p-rooms p-rooms--" ++ color;
};

let subtract = (a1, a2) => {
  let l2 = Array.to_list(a2);
  Array.to_list(a1)
  |> List.filter(e => !List.mem(e, l2))
  |> Array.of_list;
};

type ready_state = {
  id: string,
  socket: Socket.t,
  channel: Channel.t,
  rooms: room_by_room_id,
  available: array(string), /* redundant? */
  entered: array(string),
  messages: messages_by_room_id,
  text: string,
  selected: option(string)
};

type state =
  | Connecting
  | Ready(ready_state)
  | Error;

type action =
  | Connect
  | Connected(string, Socket.t, Channel.t)
  | RoomCreate
  | RoomCreated(string)
  | RoomEnter(string)
  | RoomSelect(string)
  | ReceiveRoomSetting(string, string)
  | SendRoomSetting(string)
  | Send
  | Receive(string, string, string)
  | UpdateText(string);

let component = ReasonReact.reducerComponent("App");

type res_welcome = { id: string };
type res_created = { room_id: string };
type msg_message = { source: string, room_id: string, body: string };
type msg_setting = { room_id: string, color: string };

module Decode = {
  let welcome = json =>
    Json.Decode.{
      id: json |> field("id", string)
    };

  let created = json =>
    Json.Decode.{
      room_id: json |> field("room_id", string)
    };

  let receive = json =>
    Json.Decode.{
      source: json |> field("source", string),
      room_id: json |> field("room_id", string),
      body: json |> field("body", string)
    };

  let setting = json =>
    Json.Decode.{
      room_id: json |> field("room_id", string),
      color: json |> field("color", string)
    };
};

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
            let { source, room_id, body } = Decode.receive(res);
            self.send(Receive(source, room_id, body));
          })
          |> putOn("room:setting", (res: Abstract.any) => {
            let { room_id, color } = Decode.setting(res);
            self.send(ReceiveRoomSetting(room_id, color));
          })
          |> joinChannel
          |> putReceive("ok", (res: Abstract.any) => {
            let welcome = Decode.welcome(res);
            self.send(Connected(welcome.id, socket, channel));
          });
      })
    | Connected(id, socket, channel) =>
      ReasonReact.Update(Ready({
        id,
        socket,
        channel,
        rooms: RoomMap.empty,
        available: [||],
        entered: [||],
        messages: MsgMap.empty,
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
          messages: addMsg(source, room_id, body, messages)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomCreate => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel }) =>
        push("room:create", {"source": id}, channel)
          |> putReceive("ok", (res: Abstract.any) => {
            let room_id = Decode.created(res).room_id;
            self.send(RoomCreated(room_id));
          })
          |> ignore;
        ();
      | _ => ()
      }})
    | RoomCreated(room_id) =>
      switch (state) {
      | Ready({ rooms, available, entered } as state) =>
        ReasonReact.Update(Ready({
          ...state,
          rooms: upsertRoom(room_id, "green", rooms),
          available: uniqRooms(room_id, available),
          entered: uniqRooms(room_id, entered),
          selected: Some(room_id)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomEnter(room_id) =>
      switch (state) {
      | Ready({ id, channel, entered } as state) =>
        push("room:enter", {"source": id, "room_id": room_id}, channel);
        ReasonReact.Update(Ready({
          ...state,
          entered: uniqRooms(room_id, entered),
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
    | ReceiveRoomSetting(room_id, color) =>
      switch (state) {
      | Ready({ rooms, available } as state) =>
        ReasonReact.Update(Ready({
          ...state,
          rooms: upsertRoom(room_id, color, rooms),
          available: uniqRooms(room_id, available)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | SendRoomSetting(color) => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel, selected }) =>
        switch (selected) {
        | Some(room) =>
          push("room:setting", {"source": id, "room_id": room, "color": color}, channel) |> ignore;
          /* Loopback. Update self state */
          self.send(ReceiveRoomSetting(room, color));
        | None => ()
        }
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
      | Ready({ id, rooms, available, entered, messages, text, selected }) =>
        <>
          <div className=(roomClassName(selected, rooms))>
            <header className="c-header">{ReasonReact.string("CizenChat")}</header>
            <div className="p-side-content">
              <div className="c-user">(ReasonReact.string("#" ++ id))</div>

              <button className="c-button" onClick=(_event => self.send(RoomCreate))>
                (ReasonReact.string("Create Room"))
              </button>

              <div className="c-list">
                <div className="c-list-header">(ReasonReact.string("Available Rooms"))</div>
                <div className="c-list-body">
                  (
                    subtract(available, entered)
                    |> Array.map(room =>
                      <div className="c-list-item" key=room onClick=(_event => self.send(RoomEnter(room)))>(ReasonReact.string(room))</div>
                    )
                    |> ReasonReact.array
                  )
                </div>
              </div>

              <div className="c-list">
                <div className="c-list-header">(ReasonReact.string("Joined Rooms"))</div>
                <div className="c-list-body">
                  (
                    entered
                    |> Array.map(room =>
                      <div className="c-list-item" key=room onClick=(_event => self.send(RoomSelect(room)))>(ReasonReact.string(room))</div>
                    )
                    |> ReasonReact.array
                  )
                </div>
              </div>
            </div>
          </div>
          <div className="p-chat">
            <div className="c-chat">
              (switch (selected) {
              | Some(room) =>
                <>
                  <div className="c-chat-header">
                    <span>(ReasonReact.string("Room #" ++ room))</span>
                    <div className="c-colors">
                      <div onClick=(_event => self.send(SendRoomSetting("red"))) className="c-colors-item c-colors-item--red"></div>
                      <div onClick=(_event => self.send(SendRoomSetting("green"))) className="c-colors-item c-colors-item--green"></div>
                      <div onClick=(_event => self.send(SendRoomSetting("blue"))) className="c-colors-item c-colors-item--blue"></div>
                    </div>
                  </div>
                  <div>
                    (
                      getMsg(room, messages)
                      |> Array.mapi((i, msg: message) =>
                        <div className="c-message" key=(string_of_int(i))>
                          <b>(ReasonReact.string(msg.body))</b>
                          <i>(ReasonReact.string(" by " ++ msg.avatar_id))</i>
                        </div>
                      )
                      |> ReasonReact.array
                    )
                  </div>
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

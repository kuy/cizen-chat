open Phx;

module MsgMap = Map.Make({
  type t = string;
  let compare = compare;
});

type message = {
  body: string,
  avatar_id: string
};
type messages = array(message);
type messages_by_room = MsgMap.t(messages);

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
  available: array(string),
  rooms: array(string),
  messages: messages_by_room,
  text: string,
  selected: option(string)
};

type state =
  | Connecting
  | Ready(ready_state)
  | Error;

type action =
  | Connect
  | Connected(string, Socket.t, Channel.t, array(string))
  | RoomCreate
  | RoomCreated(string)
  | RoomEnter(string)
  | RoomSelect(string)
  | Send
  | Receive(string, string, string)
  | UpdateText(string);

let component = ReasonReact.reducerComponent("App");

type welcome_response = { id: string, rooms: array(string) };
type created_response = { room_id: string };
type receive_message = { source: string, room_id: string, body: string };

module Decode = {
  let welcome = json =>
    Json.Decode.{
      id: json |> field("id", string),
      rooms: json |> field("rooms", array(string))
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
          |> joinChannel
          |> putReceive("ok", (res: Abstract.any) => {
            let welcome = Decode.welcome(res);
            self.send(Connected(welcome.id, socket, channel, welcome.rooms));
          });
      })
    | Connected(id, socket, channel, rooms) =>
      Js.log(rooms);
      ReasonReact.Update(Ready({
        id,
        socket,
        channel,
        available: rooms,
        rooms: [||],
        messages: MsgMap.empty,
        text: "",
        selected: None
      }))
    | Send => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel, rooms, text, selected }) =>
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
      | Ready({ id, socket, channel, available, rooms, messages, text, selected }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available,
          rooms,
          messages: addMsg(source, room_id, body, messages),
          text,
          selected
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
      | Ready({ id, socket, channel, available, rooms, messages, text, selected }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: Array.concat([available, [|room_id|]]),
          rooms: Array.concat([rooms, [|room_id|]]),
          messages,
          text,
          selected: Some(room_id)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomEnter(room_id) =>
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages, text, selected }) =>
        push("room:enter", {"source": id, "room_id": room_id}, channel);
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: available,
          rooms: Array.concat([rooms, [|room_id|]]),
          messages,
          text,
          selected: Some(room_id)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomSelect(room_id) =>
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages, text, selected }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available,
          rooms,
          messages,
          text,
          selected: Some(room_id)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | UpdateText(input) =>
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages, text, selected }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available,
          rooms,
          messages,
          text: input,
          selected
        }))
      | _ => ReasonReact.NoUpdate
      }
    },
  render: self => {
    <div className="p-container">
      (switch (self.state) {
      | Ready({ id, available, rooms, messages, text, selected }) =>
        <>
          <div className="p-rooms">
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
                    subtract(available, rooms)
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
                    rooms
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
                  <div className="c-chat-header">(ReasonReact.string("#" ++ room))</div>
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

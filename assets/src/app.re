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
  text: string
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
  | Send(string)
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
        text: ""
      }))
    | Send(room) => ReasonReact.SideEffects(self => {
      switch (self.state) {
      | Ready({ id, channel, rooms, text }) =>
        push("room:message", {"source": id, "room_id": room, "body": text}, channel) |> ignore;
        /* Loopback. Update self state */
        self.send(Receive(id, room, text));
        self.send(UpdateText(""));
      | _ => ()
      }})
    | Receive(source, room_id, body) =>
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages, text }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available,
          rooms,
          messages: addMsg(source, room_id, body, messages),
          text
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
      | Ready({ id, socket, channel, available, rooms, messages, text }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: Array.concat([available, [|room_id|]]),
          rooms: Array.concat([rooms, [|room_id|]]),
          messages,
          text
        }))
      | _ => ReasonReact.NoUpdate
      };
    | RoomEnter(room_id) =>
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages, text }) =>
        push("room:enter", {"source": id, "room_id": room_id}, channel);
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: available,
          rooms: Array.concat([rooms, [|room_id|]]),
          messages,
          text
        }))
      | _ => ReasonReact.NoUpdate
      };
    | UpdateText(input) =>
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages, text }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available,
          rooms,
          messages,
          text: input
        }))
      | _ => ReasonReact.NoUpdate
      };
    },
  render: self => {
    <div>
      <h1>{ReasonReact.string("CizenChat")}</h1>
      (switch (self.state) {
      | Ready({ id, available, rooms, messages, text }) =>
        <>
          <h2>(ReasonReact.string("Client ID: " ++ id))</h2>

          <button onClick=(_event => self.send(RoomCreate))>
            (ReasonReact.string("Create Room"))
          </button>

          <h2>(ReasonReact.string("Available Rooms"))</h2>
          <ul>
            (
              subtract(available, rooms)
              |> Array.map(room =>
                <li key=room>
                  <a onClick=(_event => self.send(RoomEnter(room)))>(ReasonReact.string(room))</a>
                </li>
              )
              |> ReasonReact.array
            )
          </ul>

          <h2>(ReasonReact.string("Entered Rooms"))</h2>
          <ul>
            (
              rooms
              |> Array.map(room =>
                <li key=room>
                  (ReasonReact.string(room))
                  <ul>
                    (
                      getMsg(room, messages)
                      |> Array.mapi((i, msg: message) =>
                        <li key=(string_of_int(i))>
                          <b>(ReasonReact.string(msg.body))</b>
                          <i>(ReasonReact.string(" by " ++ msg.avatar_id))</i>
                        </li>
                      )
                      |> ReasonReact.array
                    )
                    <li>
                      <input
                        placeholder="What's up?"
                        value=text
                        onKeyDown=(
                          event =>
                            if (ReactEvent.Keyboard.keyCode(event) === 13) {
                              ReactEvent.Keyboard.preventDefault(event);
                              self.send(Send(room));
                            }
                        )
                        onChange=(
                          event =>
                            self.send(UpdateText(ReactEvent.Form.target(event)##value))
                        )
                      />
                    </li>
                  </ul>
                </li>
              )
              |> ReasonReact.array
            )
          </ul>
        </>
      | _ => <h2>(ReasonReact.string("Connecting..."))</h2>
      })
    </div>;
  },
};

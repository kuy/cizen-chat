open Phx;

module MsgMap = Map.Make({
  type t = string;
  let compare = compare;
});

type messages = array(string);
type messages_by_room = MsgMap.t(messages);

let addMsg = (room, msg, map) =>
  if (MsgMap.mem(room, map)) {
    let messages = MsgMap.find(room, map);
    MsgMap.add(room, Array.concat([messages, [|msg|]]), map);
  } else {
    MsgMap.add(room, [|msg|], map);
  };

let getMsg = (room, map) =>
  try(MsgMap.find(room, map)) {
  | Not_found => [||]
  };

type ready_state = {
  id: string,
  socket: Socket.t,
  channel: Channel.t,
  available: array(string),
  rooms: array(string),
  messages: messages_by_room
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
  | Send
  | Receive(string, string, string);

let component = ReasonReact.reducerComponent("App");

let handleSend = (_event, _self) => Js.log("send");

let handleReiceive = (event, any) =>
  switch event {
  | "ok" => Js.log(("handleReiceive:" ++ event, "Joined"))
  | "error" => Js.log(("handleReiceive:" ++ event, "Failed to join channel"))
  | _ => Js.log(("handleReiceive:" ++ event, any))
};

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
          })
          |> putReceive("error", handleReiceive("error"));
      })
    | Connected(id, socket, channel, rooms) =>
      Js.log(rooms);
      ReasonReact.Update(Ready({
        id,
        socket,
        channel,
        available: rooms,
        rooms: [||],
        messages: MsgMap.empty
      }))
    | Send => ReasonReact.SideEffects(self => {
      Js.log("Send");
      switch (self.state) {
      | Ready({ id, channel, rooms }) =>
        switch (Array.to_list(rooms)) {
        | [room_id, ...rest] =>
          let text = "Greetings from ReasonReact!";
          push("room:message", {"source": id, "room_id": room_id, "body": text}, channel) |> ignore;
          /* Loopback. Update self state */
          self.send(Receive(id, room_id, text));
        | _ => ()
        }
      | _ => ()
      }})
    | Receive(source, room_id, body) =>
      Js.log("Receive");
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available,
          rooms,
          messages: addMsg(room_id, body, messages)
        }))
      | _ => ReasonReact.NoUpdate
      }
    | RoomCreate => ReasonReact.SideEffects(self => {
      Js.log("RoomCreate");
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
      Js.log("RoomCreated");
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: Array.concat([available, [|room_id|]]),
          rooms: Array.concat([rooms, [|room_id|]]),
          messages
        }))
      | _ => ReasonReact.NoUpdate
      };
    | RoomEnter(room_id) =>
      Js.log("RoomEnter");
      switch (state) {
      | Ready({ id, socket, channel, available, rooms, messages }) =>
        push("room:enter", {"source": id, "room_id": room_id}, channel);
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: available,
          rooms: Array.concat([[|room_id|], rooms]),
          messages
        }))
      | _ => ReasonReact.NoUpdate
      };
    },
  render: self => {
    <div>
      <h1>{ReasonReact.string("CizenChat")}</h1>
      (switch (self.state) {
      | Ready({ id, available, rooms, messages }) =>
        <>
          <h2>(ReasonReact.string("Client ID: " ++ id))</h2>

          <button onClick=(_event => self.send(Send))>
            (ReasonReact.string("Send Message"))
          </button>
          <button onClick=(_event => self.send(RoomCreate))>
            (ReasonReact.string("Create Room"))
          </button>

          <h2>(ReasonReact.string("Available Rooms"))</h2>
          <ul>
            (
              available
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
                      |> Array.mapi((i, msg) =>
                        <li key=(string_of_int(i))>(ReasonReact.string(msg))</li>
                      )
                      |> ReasonReact.array
                    )
                  </ul>
                </li>
              )
              |> ReasonReact.array
            )
          </ul>
        </>
      | _ => ReasonReact.string("Connecting...")
      })
    </div>;
  },
};

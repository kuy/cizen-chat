open Phx;

type ready_state = {
  id: string,
  socket: Socket.t,
  channel: Channel.t,
  available: array(string),
  rooms: array(string)
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
  | Message;

let component = ReasonReact.reducerComponent("App");

let handleSend = (_event, _self) => Js.log("send");

let handleReiceive = (event, any) =>
  switch event {
  | "ok" => Js.log(("handleReiceive:" ++ event, "Joined"))
  | "error" => Js.log(("handleReiceive:" ++ event, "Failed to join channel"))
  | _ => Js.log(("handleReiceive:" ++ event, any))
};

type join_response = { id: string, rooms: array(string) };
type create_response = { room_id: string };

module Decode = {
  let rooms = json =>
    json |> Json.Decode.(array(string));

  let welcome = json =>
    Json.Decode.{
      id: json |> field("id", string),
      rooms: json |> field("rooms", rooms)
    };

  let created = json =>
    Json.Decode.{
      room_id: json |> field("room_id", string)
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
        rooms: [||]
      }))
    | Message => ReasonReact.SideEffects(self => {
      Js.log("Message");
      switch (self.state) {
      | Ready({ id, channel, rooms }) =>
        switch (Array.to_list(rooms)) {
        | [room_id, ...rest] =>
          push("room:message", {"source": id, "room_id": room_id, "body": "Greetings from ReasonReact!"}, channel) |> ignore;
        | _ => ()
        }
      | _ => ()
      }})
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
      | Ready({ id, socket, channel, available, rooms }) =>
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: Array.concat([available, [|room_id|]]),
          rooms: Array.concat([rooms, [|room_id|]])
        }))
      | _ => ReasonReact.NoUpdate
      };
    | RoomEnter(room_id) =>
      Js.log("RoomEnter");
      switch (state) {
      | Ready({ id, socket, channel, available, rooms }) =>
        push("room:enter", {"source": id, "room_id": room_id}, channel);
        ReasonReact.Update(Ready({
          id,
          socket,
          channel,
          available: available,
          rooms: Array.concat([[|room_id|], rooms])
        }))
      | _ => ReasonReact.NoUpdate
      };
    },
  render: self => {
    <div>
      <h1>{ReasonReact.string("CizenChat")}</h1>
      (switch (self.state) {
      | Ready({ id, available, rooms }) =>
        <>
          <h2>(ReasonReact.string("Client ID: " ++ id))</h2>

          <button onClick=(_event => self.send(Message))>
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
                <li key=room>(ReasonReact.string(room))</li>
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

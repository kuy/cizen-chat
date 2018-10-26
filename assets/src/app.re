open Phx;

type ready_state = {
  id: string,
  socket: Socket.t,
  channel: Channel.t,
  rooms: array(string)
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
  | Message;

let component = ReasonReact.reducerComponent("App");

let handleSend = (_event, _self) => Js.log("send");

let handleReiceive = (event, any) =>
  switch event {
  | "ok" => Js.log(("handleReiceive:" ++ event, "Joined"))
  | "error" => Js.log(("handleReiceive:" ++ event, "Failed to join channel"))
  | _ => Js.log(("handleReiceive:" ++ event, any))
};

type join_response = { id: string };
type create_response = { room_id: string };

module Decode = {
  let welcome = json =>
    Json.Decode.{
      id: json |> field("id", string)
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
            let avatar_id = Decode.welcome(res).id;
            self.send(Connected(avatar_id, socket, channel));
          })
          |> putReceive("error", handleReiceive("error"));
      })
    | Connected(id, socket, channel) =>
      ReasonReact.Update(Ready({ id, socket, channel, rooms: [||] }))
    | Message => ReasonReact.SideEffects(self => {
        Js.log("Message");
        switch (self.state) {
        | Ready({ id, channel }) =>
          push("room:message", {"avatar_id": id, "body": "Greetings from ReasonReact!"}, channel) |> ignore;
        | _ => ()
        }
      })
    | RoomCreate => ReasonReact.SideEffects(self => {
        Js.log("RoomCreate");
        switch (self.state) {
        | Ready({ id, channel }) =>
          push("room:create", {"avatar_id": id}, channel)
            |> putReceive("ok", (res: Abstract.any) => {
              let room_id = Decode.created(res).room_id;
              self.send(RoomCreated(room_id));
            })
            |> ignore;
          ();
        | _ => ()
        }
      })
    | RoomCreated(room_id) =>
      switch (state) {
      | Ready({ id, socket, channel, rooms }) =>
        Js.log(rooms);
        ReasonReact.Update(Ready({ id, socket, channel, rooms: Array.concat([rooms, [|room_id|]])}))
      | _ => ReasonReact.NoUpdate
      };
    },
  render: self => {
    <div>
      <h1>{ReasonReact.string("CizenChat")}</h1>
      <button onClick=(_event => self.send(Message))>
        (ReasonReact.string("Send Message"))
      </button>
      <button onClick=(_event => self.send(RoomCreate))>
        (ReasonReact.string("Create Room"))
      </button>

      <ul>
        (switch (self.state) {
        | Ready({ rooms }) =>
          rooms
          |> Array.map(room =>
            <li key=room>(ReasonReact.string(room))</li>
          )
          |> ReasonReact.array
        | _ => ReasonReact.string("Connecting...")
        })
      </ul>
    </div>;
  },
};

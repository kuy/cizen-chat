open Phx;

type state =
  | Connecting
  | Ready(string, Socket.t, Channel.t)
  | Error;

type action =
  | Connect
  | Connected(string, Socket.t, Channel.t)
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

module Decode = {
  let response = json =>
    Json.Decode.{
      id: json |> field("id", string)
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
                     |> connectSocket
        let channel = socket
                      |> initChannel("lounge:hello");
        let _ =
          channel
          |> joinChannel
          |> putReceive("ok", (res: Abstract.any) => {
            let avatar_id = Decode.response(res).id;
            self.send(Connected(avatar_id, socket, channel));
          })
          |> putReceive("error", handleReiceive("error"));
      })
    | Connected(id, socket, channel) =>
      ReasonReact.Update(Ready(id, socket, channel))
    | Message => ReasonReact.SideEffects(self => {
        Js.log("Message");
        switch (self.state) {
        | Ready(id, _socket, channel) =>
          push("room:message", {"avatar_id": id, "body": "Greetings from ReasonReact!"}, channel);
          ();
        | _ => ()
        }
      })
    },
  render: self => {
    <div>
      <h1>{ReasonReact.string("CizenChat")}</h1>
      <button onClick=(_event => self.send(Message))>
        (ReasonReact.string("Send"))
      </button>
    </div>;
  },
};

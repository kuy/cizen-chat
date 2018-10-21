open Phx

Js.log("hello cizen!");

let handleReiceive = (event, any) =>
  switch event {
  | "ok" => Js.log(("handleReiceive:" ++ event, "Joined"))
  | "error" => Js.log(("handleReiceive:" ++ event, "Failed to join channel"))
  | _ => Js.log(("handleReiceive:" ++ event, any))
};

let handleEvent = (event, response) => {
  let _ = Js.log(("handleEvent:" ++ event, response));
  ();
};

let handleSyncState = (response) => {
  let _ = Js.log(("handleSyncState", response));
  let _presences = Presence.syncState(Js.Dict.empty(), response);
  ();
};

let handleSyncDiff = (diff) => {
  let _ = Js.log(("handleSyncDiff:diff", diff));
  let presences = Presence.syncDiff(Js.Dict.empty(), diff);
  let _ = Js.log(("handleSyncDiff:presences", presences));
  ();
};

{
  let socket = initSocket("/socket")
               |> connectSocket
               |> putOnClose(() => Js.log("Socket closed"));
  let channel = socket
                |> initChannel("room:hello");
  let _ =
    channel
    |> putOn("from_server", handleEvent("from:server"))
    |> putOnSyncState(handleSyncState)
    |> putOnsyncDiff(handleSyncDiff)
    |> joinChannel
    |> putReceive("ok", handleReiceive("ok"))
    |> putReceive("error", handleReiceive("error"));
  push("new:message", {"body": "Hello, Elixir! This is a greeting from Reason!"}, channel);
}

type welcome = { id: string, name: string };
type message = { source: string, room_id: string, body: string };
type setting = { room_id: string, name: string, color: string };
type profile = { avatar_id: string, name: string };

let welcome = json =>
  Json.Decode.{
    id: json |> field("id", string),
    name: json |> field("name", string)
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
    name: json |> field("name", string),
    color: json |> field("color", string)
  };

let profile = json =>
  Json.Decode.{
    avatar_id: json |> field("avatar_id", string),
    name: json |> field("name", string)
  };

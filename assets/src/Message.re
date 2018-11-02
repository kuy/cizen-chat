open MessageMap;

type t = {
  body: string,
  avatar_id: string
};

let addMsg = (avatar_id, room, body, map) => {
  let msg = { body, avatar_id };
  if (MessageMap.mem(room, map)) {
    let messages = MessageMap.find(room, map);
    MessageMap.add(room, Array.concat([messages, [|msg|]]), map);
  } else {
    MessageMap.add(room, [|msg|], map);
  };
};

let getMsg = (room, map) =>
  try(MessageMap.find(room, map)) {
  | Not_found => [||]
  };

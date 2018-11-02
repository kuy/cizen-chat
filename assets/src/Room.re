type t = {
  id: string,
  name: string,
  color: string
};

module RoomMap = Map.Make({
  type t = string;
  let compare = compare;
});

type by_room_id = RoomMap.t(t);

let uniqRooms = (room_id, rooms) => {
  let rooms_l = Array.to_list(rooms);
  if (List.mem(room_id, rooms_l)) {
    rooms;
  } else {
    Array.concat([rooms, [|room_id|]]);
  };
};

let upsertRoom = (room_id, name, color, rooms) => {
  let room = { id: room_id, name, color };
  RoomMap.add(room_id, room, rooms);
};

let getRoomName = (room_id, rooms) => {
  try(RoomMap.find(room_id, rooms).name) {
  | Not_found => "Unknown Room"
  }
};

let getRoomColor = (room_id, rooms) => {
  try(RoomMap.find(room_id, rooms).color) {
  | Not_found => "green"
  }
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

let byId = (room_id, rooms) =>
  try(Some(RoomMap.find(room_id, rooms))) {
  | Not_found => None
  };

let byIds = (room_ids, rooms) =>
  Belt.Array.keepMap(room_ids, room_id => byId(room_id, rooms));

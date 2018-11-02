open MessageMap;

type t = array(Message.t);
type by_room_id_t = MessageMap.t(t);

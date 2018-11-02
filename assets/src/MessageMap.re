module MessageMap = Map.Make({
  type t = string;
  let compare = compare;
});

let component = ReasonReact.statelessComponent("RoomList");

let make = (~title, ~rooms, ~handleSelect, _children) => {
  ...component,
  render: self => {
    <div className="c-list">
      <div className="c-list-header">(ReasonReact.string(title))</div>
      <div className="c-list-body">
        (
          rooms
          |> Array.map((room: Room.t) =>
            <div
              className="c-list-item"
              key=(room.id)
              onClick=(_event => handleSelect(room.id))
            >
              (ReasonReact.string(room.name))
            </div>
          )
          |> ReasonReact.array
        )
      </div>
    </div>;
  },
};

type editing_state = {
  text: string
};

type state =
  | Fixed
  | Editing(editing_state);

type action =
  | Toggle
  | Update(string);

let component = ReasonReact.reducerComponent("RoomTitle");

let make = (~name, ~handleChange, _children) => {
  ...component,
  initialState: () => Fixed,
  reducer: (action, state) =>
    switch (action) {
    | Toggle =>
      switch (state) {
      | Fixed => ReasonReact.Update(Editing({ text: name }))
      | Editing({ text }) => ReasonReact.Update(Fixed)
      }
    | Update(text) =>
      ReasonReact.Update(Editing({ text: text }))
    },
  render: self => {
    <div className="c-room-title">
      (switch (self.state) {
      | Fixed =>
        <>
          <span className="c-room-title-text">(ReasonReact.string("Room #" ++ name))</span>
          <span
            className="c-room-title-action"
            onClick=(_event => Toggle |> self.send)
          >
            (ReasonReact.string("Edit"))
          </span>
        </>
      | Editing({ text }) =>
        <input
          value=text
          onKeyDown=(
            event =>
              if (ReactEvent.Keyboard.keyCode(event) === 13) {
                ReactEvent.Keyboard.preventDefault(event);
                handleChange(text);
                Toggle |> self.send;
              }
          )
          onChange=(
            event =>
              Update(ReactEvent.Form.target(event)##value) |> self.send
          )
        />
      })
    </div>;
  },
};

type editing_state = {
  text: string
};

type state =
  | Fixed
  | Editing(editing_state);

type action =
  | Toggle
  | Update(string);

let component = ReasonReact.reducerComponent("InPlaceEdit");

let make = (~name, ~text, ~handleChange, _children) => {
  ...component,
  initialState: () => Fixed,
  reducer: (action, state) =>
    switch (action) {
    | Toggle =>
      switch (state) {
      | Fixed => ReasonReact.Update(Editing({ text: text }))
      | Editing({ text }) => ReasonReact.Update(Fixed)
      }
    | Update(str) =>
      ReasonReact.Update(Editing({ text: str }))
    },
  render: self => {
    <div className=("c-iedit-" ++ name)>
      (switch (self.state) {
      | Fixed =>
        <>
          <span className=("c-iedit-" ++ name ++ "-content")>(ReasonReact.string(text))</span>
          <span
            className=("c-iedit-" ++ name ++ "-action")
            onClick=(_event => Toggle |> self.send)
          >
            (ReasonReact.string("Edit"))
          </span>
        </>
      | Editing({ text as str }) =>
        <input
          value=str
          onKeyDown=(
            event =>
              if (ReactEvent.Keyboard.keyCode(event) === 13) {
                ReactEvent.Keyboard.preventDefault(event);
                handleChange(str);
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

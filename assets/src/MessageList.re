open Message;

let component = ReasonReact.statelessComponent("MessageList");

let make = (~messages, _children) => {
  ...component,
  render: self => {
    <div className="c-messages">
      (
        messages
        |> Array.mapi((i: int, msg: Message.t) =>
          <div className="c-message" key=(string_of_int(i))>
            <b>(ReasonReact.string(msg.body))</b>
            <i>(ReasonReact.string(" by " ++ msg.avatar_id))</i>
          </div>
        )
        |> ReasonReact.array
      )
    </div>;
  },
};

let component = ReasonReact.statelessComponent("ThemeChanger");

let make = (~handleChange, _children) => {
  ...component,
  render: self => {
    <div className="c-colors">
      <div onClick=(_event => handleChange("red")) className="c-colors-item c-colors-item--red"></div>
      <div onClick=(_event => handleChange("green")) className="c-colors-item c-colors-item--green"></div>
      <div onClick=(_event => handleChange("blue")) className="c-colors-item c-colors-item--blue"></div>
      <div onClick=(_event => handleChange("yellow")) className="c-colors-item c-colors-item--yellow"></div>
    </div>;
  },
};

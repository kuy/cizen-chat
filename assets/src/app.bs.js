// Generated by BUCKLESCRIPT VERSION 4.0.6, PLEASE EDIT WITH CARE
'use strict';

var $$Map = require("bs-platform/lib/js/map.js");
var Phx = require("bucklescript-phx/src/phx.js");
var $$Array = require("bs-platform/lib/js/array.js");
var Block = require("bs-platform/lib/js/block.js");
var Curry = require("bs-platform/lib/js/curry.js");
var React = require("react");
var Caml_obj = require("bs-platform/lib/js/caml_obj.js");
var Json_decode = require("@glennsl/bs-json/src/Json_decode.bs.js");
var ReasonReact = require("reason-react/src/ReasonReact.js");
var Caml_builtin_exceptions = require("bs-platform/lib/js/caml_builtin_exceptions.js");

var compare = Caml_obj.caml_compare;

var MsgMap = $$Map.Make(/* module */[/* compare */compare]);

function addMsg(room, msg, map) {
  if (Curry._2(MsgMap[/* mem */2], room, map)) {
    var messages = Curry._2(MsgMap[/* find */21], room, map);
    return Curry._3(MsgMap[/* add */3], room, $$Array.concat(/* :: */[
                    messages,
                    /* :: */[
                      /* array */[msg],
                      /* [] */0
                    ]
                  ]), map);
  } else {
    return Curry._3(MsgMap[/* add */3], room, /* array */[msg], map);
  }
}

function getMsg(room, map) {
  try {
    return Curry._2(MsgMap[/* find */21], room, map);
  }
  catch (exn){
    if (exn === Caml_builtin_exceptions.not_found) {
      return /* array */[];
    } else {
      throw exn;
    }
  }
}

var component = ReasonReact.reducerComponent("App");

function handleSend(_, _$1) {
  console.log("send");
  return /* () */0;
}

function handleReiceive($$event, any) {
  switch ($$event) {
    case "error" : 
        console.log(/* tuple */[
              "handleReiceive:" + $$event,
              "Failed to join channel"
            ]);
        return /* () */0;
    case "ok" : 
        console.log(/* tuple */[
              "handleReiceive:" + $$event,
              "Joined"
            ]);
        return /* () */0;
    default:
      console.log(/* tuple */[
            "handleReiceive:" + $$event,
            any
          ]);
      return /* () */0;
  }
}

function welcome(json) {
  return /* record */[
          /* id */Json_decode.field("id", Json_decode.string, json),
          /* rooms */Json_decode.field("rooms", (function (param) {
                  return Json_decode.array(Json_decode.string, param);
                }), json)
        ];
}

function created(json) {
  return /* record */[/* room_id */Json_decode.field("room_id", Json_decode.string, json)];
}

function receive(json) {
  return /* record */[
          /* source */Json_decode.field("source", Json_decode.string, json),
          /* room_id */Json_decode.field("room_id", Json_decode.string, json),
          /* body */Json_decode.field("body", Json_decode.string, json)
        ];
}

var Decode = /* module */[
  /* welcome */welcome,
  /* created */created,
  /* receive */receive
];

function make() {
  return /* record */[
          /* debugName */component[/* debugName */0],
          /* reactClassInternal */component[/* reactClassInternal */1],
          /* handedOffState */component[/* handedOffState */2],
          /* willReceiveProps */component[/* willReceiveProps */3],
          /* didMount */(function (self) {
              return Curry._1(self[/* send */3], /* Connect */0);
            }),
          /* didUpdate */component[/* didUpdate */5],
          /* willUnmount */component[/* willUnmount */6],
          /* willUpdate */component[/* willUpdate */7],
          /* shouldUpdate */component[/* shouldUpdate */8],
          /* render */(function (self) {
              var match = self[/* state */1];
              var tmp;
              if (typeof match === "number") {
                tmp = "Connecting...";
              } else {
                var match$1 = match[0];
                var messages = match$1[/* messages */5];
                tmp = React.createElement(React.Fragment, undefined, React.createElement("h2", undefined, "Client ID: " + match$1[/* id */0]), React.createElement("button", {
                          onClick: (function () {
                              return Curry._1(self[/* send */3], /* Send */2);
                            })
                        }, "Send Message"), React.createElement("button", {
                          onClick: (function () {
                              return Curry._1(self[/* send */3], /* RoomCreate */1);
                            })
                        }, "Create Room"), React.createElement("h2", undefined, "Available Rooms"), React.createElement("ul", undefined, $$Array.map((function (room) {
                                return React.createElement("li", {
                                            key: room
                                          }, React.createElement("a", {
                                                onClick: (function () {
                                                    return Curry._1(self[/* send */3], /* RoomEnter */Block.__(2, [room]));
                                                  })
                                              }, room));
                              }), match$1[/* available */3])), React.createElement("h2", undefined, "Entered Rooms"), React.createElement("ul", undefined, $$Array.map((function (room) {
                                return React.createElement("li", {
                                            key: room
                                          }, room, React.createElement("ul", undefined, $$Array.mapi((function (i, msg) {
                                                      return React.createElement("li", {
                                                                  key: String(i)
                                                                }, msg);
                                                    }), getMsg(room, messages))));
                              }), match$1[/* rooms */4])));
              }
              return React.createElement("div", undefined, React.createElement("h1", undefined, "CizenChat"), tmp);
            }),
          /* initialState */(function () {
              return /* Connecting */0;
            }),
          /* retainedProps */component[/* retainedProps */11],
          /* reducer */(function (action, state) {
              if (typeof action === "number") {
                switch (action) {
                  case 0 : 
                      return /* SideEffects */Block.__(1, [(function (self) {
                                    var eta = Phx.initSocket(undefined, "/socket");
                                    var socket = Phx.connectSocket(undefined, eta);
                                    var channel = (function (eta) {
                                          return Phx.initChannel("lounge:hello", undefined, eta);
                                        })(socket);
                                    var eta$1 = Phx.putOn("room:message", (function (res) {
                                            var match = receive(res);
                                            return Curry._1(self[/* send */3], /* Receive */Block.__(3, [
                                                          match[/* source */0],
                                                          match[/* room_id */1],
                                                          match[/* body */2]
                                                        ]));
                                          }), channel);
                                    Phx.putReceive("error", (function (param) {
                                            return handleReiceive("error", param);
                                          }), Phx.putReceive("ok", (function (res) {
                                                var welcome$1 = welcome(res);
                                                return Curry._1(self[/* send */3], /* Connected */Block.__(0, [
                                                              welcome$1[/* id */0],
                                                              socket,
                                                              channel,
                                                              welcome$1[/* rooms */1]
                                                            ]));
                                              }), Phx.joinChannel(undefined, eta$1)));
                                    return /* () */0;
                                  })]);
                  case 1 : 
                      return /* SideEffects */Block.__(1, [(function (self) {
                                    console.log("RoomCreate");
                                    var match = self[/* state */1];
                                    if (typeof match === "number") {
                                      return /* () */0;
                                    } else {
                                      var match$1 = match[0];
                                      Phx.putReceive("ok", (function (res) {
                                              var room_id = created(res)[/* room_id */0];
                                              return Curry._1(self[/* send */3], /* RoomCreated */Block.__(1, [room_id]));
                                            }), Phx.push("room:create", {
                                                source: match$1[/* id */0]
                                              }, undefined, match$1[/* channel */2]));
                                      return /* () */0;
                                    }
                                  })]);
                  case 2 : 
                      return /* SideEffects */Block.__(1, [(function (self) {
                                    console.log("Send");
                                    var match = self[/* state */1];
                                    if (typeof match === "number") {
                                      return /* () */0;
                                    } else {
                                      var match$1 = match[0];
                                      var id = match$1[/* id */0];
                                      var match$2 = $$Array.to_list(match$1[/* rooms */4]);
                                      if (match$2) {
                                        var room_id = match$2[0];
                                        var text = "Greetings from ReasonReact!";
                                        Phx.push("room:message", {
                                              source: id,
                                              room_id: room_id,
                                              body: text
                                            }, undefined, match$1[/* channel */2]);
                                        return Curry._1(self[/* send */3], /* Receive */Block.__(3, [
                                                      id,
                                                      room_id,
                                                      text
                                                    ]));
                                      } else {
                                        return /* () */0;
                                      }
                                    }
                                  })]);
                  
                }
              } else {
                switch (action.tag | 0) {
                  case 0 : 
                      var rooms = action[3];
                      console.log(rooms);
                      return /* Update */Block.__(0, [/* Ready */[/* record */[
                                    /* id */action[0],
                                    /* socket */action[1],
                                    /* channel */action[2],
                                    /* available */rooms,
                                    /* rooms : array */[],
                                    /* messages */MsgMap[/* empty */0]
                                  ]]]);
                  case 1 : 
                      var room_id = action[0];
                      console.log("RoomCreated");
                      if (typeof state === "number") {
                        return /* NoUpdate */0;
                      } else {
                        var match = state[0];
                        return /* Update */Block.__(0, [/* Ready */[/* record */[
                                      /* id */match[/* id */0],
                                      /* socket */match[/* socket */1],
                                      /* channel */match[/* channel */2],
                                      /* available */$$Array.concat(/* :: */[
                                            match[/* available */3],
                                            /* :: */[
                                              /* array */[room_id],
                                              /* [] */0
                                            ]
                                          ]),
                                      /* rooms */$$Array.concat(/* :: */[
                                            match[/* rooms */4],
                                            /* :: */[
                                              /* array */[room_id],
                                              /* [] */0
                                            ]
                                          ]),
                                      /* messages */match[/* messages */5]
                                    ]]]);
                      }
                  case 2 : 
                      var room_id$1 = action[0];
                      console.log("RoomEnter");
                      if (typeof state === "number") {
                        return /* NoUpdate */0;
                      } else {
                        var match$1 = state[0];
                        var channel = match$1[/* channel */2];
                        var id = match$1[/* id */0];
                        Phx.push("room:enter", {
                              source: id,
                              room_id: room_id$1
                            }, undefined, channel);
                        return /* Update */Block.__(0, [/* Ready */[/* record */[
                                      /* id */id,
                                      /* socket */match$1[/* socket */1],
                                      /* channel */channel,
                                      /* available */match$1[/* available */3],
                                      /* rooms */$$Array.concat(/* :: */[
                                            /* array */[room_id$1],
                                            /* :: */[
                                              match$1[/* rooms */4],
                                              /* [] */0
                                            ]
                                          ]),
                                      /* messages */match$1[/* messages */5]
                                    ]]]);
                      }
                  case 3 : 
                      console.log("Receive");
                      if (typeof state === "number") {
                        return /* NoUpdate */0;
                      } else {
                        var match$2 = state[0];
                        return /* Update */Block.__(0, [/* Ready */[/* record */[
                                      /* id */match$2[/* id */0],
                                      /* socket */match$2[/* socket */1],
                                      /* channel */match$2[/* channel */2],
                                      /* available */match$2[/* available */3],
                                      /* rooms */match$2[/* rooms */4],
                                      /* messages */addMsg(action[1], action[2], match$2[/* messages */5])
                                    ]]]);
                      }
                  
                }
              }
            }),
          /* jsElementWrapped */component[/* jsElementWrapped */13]
        ];
}

exports.MsgMap = MsgMap;
exports.addMsg = addMsg;
exports.getMsg = getMsg;
exports.component = component;
exports.handleSend = handleSend;
exports.handleReiceive = handleReiceive;
exports.Decode = Decode;
exports.make = make;
/* MsgMap Not a pure module */

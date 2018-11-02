# Room

defmodule CizenChat.Events.Room.Create do
  defstruct [:source]

  use Cizen.Request
  defresponse Done, :create_id do
    defstruct [:create_id, :room_id, :name, :color]
  end
end

defmodule CizenChat.Events.Room.Delete do
  defstruct [:source, :room_id]
end

defmodule CizenChat.Events.Room.Enter do
  defstruct [:source, :room_id]
end

defmodule CizenChat.Events.Room.Leave do
  defstruct [:source, :room_id]
end

defmodule CizenChat.Events.Room.Setting do
  defstruct [:source, :room_id, :name, :color]
end

defmodule CizenChat.Events.Room.Advertise do
  defstruct [:joiner_id]
end

defmodule CizenChat.Events.Room.SelfIntro do
  defstruct [:room_id]

  use Cizen.Request
  defresponse Explain, :self_intro_id do
    defstruct [:self_intro_id, :name, :color]
  end
end

defmodule CizenChat.Events.Room.Message do
  defstruct [:source, :dest, :room_id, :text]
end

defmodule CizenChat.Events.Room.Message.Transport do
  defstruct [:source, :dest, :direction, :room_id, :text]
end

# Transport: An event for internal communication. Cizen Automata <=> Phoenix Channel.

defmodule CizenChat.Events.Transport do
  defstruct [:source, :dest, :direction, :body]

  import Cizen.EventBodyFilter
  defeventbodyfilter RoomIDFilter, :room_id do
    @impl true
    def test(%{value: value}, wrapper) do
      case wrapper do
        %CizenChat.Events.Transport{source: _source, dest: _dest, direction: _direction, body: body} ->
          case body do
            %CizenChat.Events.Room.Setting{source: _source, room_id: room_id, name: _name, color: _color} ->
              value == room_id
            _ -> false
          end
        _ -> false
      end
    end
  end
end

# Lounge

defmodule CizenChat.Events.Lounge.Join do
  defstruct []

  use Cizen.Request
  defresponse Welcome, :join_id do
    defstruct [:join_id, :avatar_id, :rooms]
  end
end

# Filters

defmodule CizenChat.Events do
  import Cizen.EventBodyFilter
  defeventbodyfilter SourceFilter, :source
  defeventbodyfilter DestFilter, :dest
  defeventbodyfilter RoomIDFilter, :room_id
  defeventbodyfilter DirectionFilter, :direction
end

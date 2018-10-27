# Lounge

defmodule CizenChat.Events.Lounge.Join do
  defstruct []

  use Cizen.Request
  defresponse Welcome, :join_id do
    defstruct [:join_id, :avatar_id, :rooms]
  end
end

# Room

defmodule CizenChat.Events.Room do
  defstruct []

  import Cizen.EventBodyFilter
  defeventbodyfilter SourceFilter, :source
  defeventbodyfilter DestFilter, :dest
  defeventbodyfilter RoomIDFilter, :room_id
  defeventbodyfilter DirectionFilter, :direction
end

defmodule CizenChat.Events.Room.Create do
  defstruct [:source]

  use Cizen.Request
  defresponse Done, :create_id do
    defstruct [:create_id, :room_id]
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

defmodule CizenChat.Events.Room.Message do
  defstruct [:source, :dest, :room_id, :text]
end

defmodule CizenChat.Events.Room.Message.Transport do
  defstruct [:source, :dest, :direction, :room_id, :text]
end

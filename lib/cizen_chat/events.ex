# Lounge

defmodule CizenChat.Events.Lounge.Join do
  defstruct []

  use Cizen.Request
  defresponse Welcome, :join_id do
    defstruct [:join_id, :avatar_id]
  end
end

# Room

defmodule CizenChat.Events.Room do
  defstruct []

  import Cizen.EventBodyFilter
  defeventbodyfilter AvatarIDFilter, :avatar_id
end

defmodule CizenChat.Events.Room.Create do
  defstruct [:avatar_id]

  use Cizen.Request
  defresponse Done, :create_id do
    defstruct [:create_id, :room_id]
  end
end

defmodule CizenChat.Events.Room.Delete do
  defstruct [:room_id]
end

defmodule CizenChat.Events.Room.Enter do
  defstruct [:room_id]
end

defmodule CizenChat.Events.Room.Leave do
  defstruct [:room_id]
end

defmodule CizenChat.Events.Room.Message do
  defstruct [:avatar_id, :room_id, :text]
end

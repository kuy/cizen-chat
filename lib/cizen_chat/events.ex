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

# Avatar

defmodule CizenChat.Events.Avatar.SelfIntro do
  defstruct [:avatar_id]

  use Cizen.Request
  defresponse Explain, :self_intro_id do
    defstruct [:self_intro_id, :name]
  end
end

defmodule CizenChat.Events.Avatar.Profile do
  defstruct [:source, :name]
end

# Transport: A wrapper event for inter-subsystem communication. Cizen Automata <=> Phoenix Channels.

defmodule CizenChat.Events.Transport do
  defstruct [:dest, :direction, :body]
end

# Lounge

defmodule CizenChat.Events.Lounge.Join do
  defstruct []

  use Cizen.Request
  defresponse Welcome, :join_id do
    defstruct [:join_id, :avatar_id, :avatar_name]
  end
end

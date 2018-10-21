defmodule CizenChat.Events.Join do
  defstruct []

  use Cizen.Request
  defresponse Welcome, :join_id do
    defstruct [:join_id, :avatar_id]
  end
end

defmodule CizenChat.Events.Message do
  defstruct [:avatar_id, :text]

  import Cizen.EventBodyFilter
  defeventbodyfilter AvatarIDFilter, :avatar_id
end

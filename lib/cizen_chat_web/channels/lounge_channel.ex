alias Cizen.Effects.{Dispatch, Request}
alias CizenChat.Events.{Lounge, Room}

defmodule CizenChatWeb.LoungeChannel do
  use Phoenix.Channel
  use Cizen.Effectful

  def join("lounge:hello", _message, socket) do
    avatar_id = handle fn id ->
      welcome_event = perform id, %Request{body: %Lounge.Join{}}
      welcome_event.body.avatar_id
    end
    {:ok, %{id: avatar_id}, socket}
  end

  def join("lounge:" <> _private_room_id, _params, _socket) do
    {:error, %{reason: "unauthorized"}}
  end

  def handle_in("room:create", %{"avatar_id" => avatar_id}, socket) do
    IO.puts("room:create: by #{avatar_id}")
    room_id = handle fn id ->
      done_event = perform id, %Request{
        body: %Room.Create{avatar_id: avatar_id}
      }
      done_event.body.room_id
    end
    {:reply, {:ok, %{room_id: room_id}}, socket}
  end

  def handle_in("room:message", %{"avatar_id" => avatar_id, "room_id" => room_id, "body" => body}, socket) do
    IO.puts("room:message: #{body} from #{avatar_id} at #{room_id}")
    handle fn id ->
      perform id, %Dispatch{
        body: %Room.Message{avatar_id: avatar_id, room_id: room_id, text: body}
      }
    end
    {:reply, :ok, socket}
  end
end

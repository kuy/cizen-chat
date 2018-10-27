alias Cizen.Effects.{Dispatch, Request}
alias CizenChat.Events.{Lounge, Room}

defmodule CizenChatWeb.LoungeChannel do
  use Phoenix.Channel
  use Cizen.Effectful

  def join("lounge:hello", _message, socket) do
    {avatar_id, rooms} = handle fn id ->
      welcome_event = perform id, %Request{body: %Lounge.Join{}}
      {welcome_event.body.avatar_id, welcome_event.body.rooms}
    end
    {:ok, %{id: avatar_id, rooms: rooms}, socket}
  end

  def join("lounge:" <> _private_room_id, _params, _socket) do
    {:error, %{reason: "unauthorized"}}
  end

  def handle_in("room:create", %{"source" => source}, socket) do
    IO.puts("Channel#room:create: by #{source}")
    room_id = handle fn id ->
      done_event = perform id, %Request{
        body: %Room.Create{source: source}
      }
      done_event.body.room_id
    end
    {:reply, {:ok, %{room_id: room_id}}, socket}
  end

  def handle_in("room:enter", %{"source" => source, "room_id" => room_id}, socket) do
    IO.puts("Channel#room:enter: to=#{room_id}, by=#{source}")
    handle fn id ->
      perform id, %Dispatch{
        body: %Room.Enter{source: source, room_id: room_id}
      }
    end
    {:noreply, socket}
  end

  def handle_in("room:message", %{"source" => source, "room_id" => room_id, "body" => body}, socket) do
    IO.puts("Channel#room:message: #{body} from #{source} at #{room_id}")
    handle fn id ->
      perform id, %Dispatch{
        body: %Room.Message.Transport{
          source: source,
          dest: source,
          direction: :incoming,
          room_id: room_id,
          text: body
        }
      }
    end
    {:reply, :ok, socket}
  end
end

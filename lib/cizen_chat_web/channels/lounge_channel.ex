alias Cizen.Effects.{Subscribe, Dispatch, Request, Receive, Start}
alias CizenChat.Events.{Lounge, Room}

defmodule CizenChatWeb.TransportAutomaton do
  alias Phoenix.Channel
  use Cizen.Automaton

  alias Cizen.EventFilter

  defstruct [:socket, :avatar_id]

  @impl true
  def spawn(id, %__MODULE__{socket: socket, avatar_id: avatar_id}) do
    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Message.Transport,
        event_body_filters: [
          %Room.DestFilter{value: avatar_id},
          %Room.DirectionFilter{value: :outgoing}
        ]
      )
    }

    %{socket: socket}
  end

  @impl true
  def yield(id, %{socket: socket}) do
    event = perform id, %Receive{}
    case event.body do
      %Room.Message.Transport{source: source, dest: _dest, direction: _direction, room_id: room_id, text: text} ->
        IO.puts("TransportAutomaton <= Room.Message.Transport: '#{text}' from #{source} at #{room_id}")
        Channel.push socket, "room:message", %{source: source, room_id: room_id, body: text}
    end
    %{socket: socket}
  end
end

defmodule CizenChatWeb.LoungeChannel do
  use Phoenix.Channel
  use Cizen.Effectful

  def join("lounge:hello", _message, socket) do
    {avatar_id, rooms} = handle fn id ->
      welcome_event = perform id, %Request{body: %Lounge.Join{}}
      {welcome_event.body.avatar_id, welcome_event.body.rooms}
    end

    send(self(), {:after_join, avatar_id})

    {:ok, %{id: avatar_id, rooms: rooms}, socket}
  end

  def join("lounge:" <> _private_room_id, _params, _socket) do
    {:error, %{reason: "unauthorized"}}
  end

  def handle_info({:after_join, avatar_id}, socket) do
    push socket, "room:hoge", %{hoge: :hoge}

    handle fn id ->
      perform id, %Start{
        saga: %CizenChatWeb.TransportAutomaton{socket: socket, avatar_id: avatar_id}
      }
    end

    {:noreply, socket}
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

alias Cizen.Effects.{Subscribe, Dispatch, Request, Receive, Start}
alias CizenChat.Events
alias CizenChat.Events.{Transport, Lounge, Room}

defmodule CizenChatWeb.Gateway do
  alias Phoenix.Channel
  use Cizen.Automaton

  alias Cizen.EventFilter

  defstruct [:socket, :avatar_id]

  @impl true
  def spawn(id, %__MODULE__{socket: socket, avatar_id: avatar_id}) do
    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Transport,
        event_body_filters: [
          %Events.DestFilter{value: avatar_id},
          %Events.DirectionFilter{value: :outgoing}
        ]
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Message.Transport,
        event_body_filters: [
          %Events.DestFilter{value: avatar_id},
          %Events.DirectionFilter{value: :outgoing}
        ]
      )
    }

    # FIXME: Advertising should be requested from Cizen, not the external layer
    perform id, %Dispatch{
      body: %Room.Advertise{
        joiner_id: avatar_id
      }
    }

    %{
      avatar_id: avatar_id,
      socket: socket
    }
  end

  @impl true
  def yield(id, state) do
    IO.puts("Gateway[#{state.avatar_id}]")
    event = perform id, %Receive{}
    case event.body do
      %Transport{source: _source, dest: _dest, direction: _direction, body: body} ->
        case body do
          %Room.Setting{source: _source, room_id: room_id, color: color} ->
            IO.puts("Gateway[#{state.avatar_id}] <= Transport(Room.Setting): room=#{room_id}")
            Channel.push state.socket, "room:setting", %{room_id: room_id, color: color}
        end
      %Room.Message.Transport{source: source, dest: _dest, direction: _direction, room_id: room_id, text: text} ->
        IO.puts("Gateway[#{state.avatar_id}] <= Room.Message.Transport: '#{text}' from #{source} at #{room_id}")
        Channel.push state.socket, "room:message", %{source: source, room_id: room_id, body: text}
    end
    state
  end
end

defmodule CizenChatWeb.LoungeChannel do
  use Phoenix.Channel
  use Cizen.Effectful

  def join("lounge:hello", _message, socket) do
    avatar_id = handle fn id ->
      welcome_event = perform id, %Request{body: %Lounge.Join{}}
      welcome_event.body.avatar_id
    end

    send(self(), {:after_join, avatar_id})

    {:ok, %{id: avatar_id}, socket}
  end

  def join("lounge:" <> _private_room_id, _params, _socket) do
    {:error, %{reason: "unauthorized"}}
  end

  def handle_info({:after_join, avatar_id}, socket) do
    handle fn id ->
      perform id, %Start{
        saga: %CizenChatWeb.Gateway{socket: socket, avatar_id: avatar_id}
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

  def handle_in("room:setting", %{"source" => source, "room_id" => room_id, "color" => color}, socket) do
    IO.puts("Channel#room:setting: to=#{room_id}, color=#{color}, by=#{source}")
    handle fn id ->
      perform id, %Dispatch{
        body: %Transport{
          source: id, # Gateway's saga id
          dest: source,
          direction: :incoming,
          body: %Room.Setting{
            source: source,
            room_id: room_id,
            color: color
          }
        }
      }
    end
    {:noreply, socket}
  end
end

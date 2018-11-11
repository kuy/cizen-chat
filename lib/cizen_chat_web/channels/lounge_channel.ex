alias Cizen.Effects.{Dispatch, Request}
alias Cizen.{Dispatcher, Event, Filter}
alias CizenChat.Events.{Transport, Lounge, Room, Avatar}

defmodule CizenChatWeb.LoungeChannel do
  use Phoenix.Channel
  use Cizen.Effectful

  def join("lounge:hello", _message, socket) do
    %{body: %{avatar_id: id, avatar_name: name}} = handle fn id ->
      perform id, %Request{body: %Lounge.Join{}}
    end

    Dispatcher.listen(Filter.new(
      fn %Event{body: %Transport{dest: dest, direction: dir}} ->
        dest == id and dir == :outgoing
      end
    ))

    {:ok, %{id: id, name: name}, socket}
  end

  def join("lounge:" <> _private_room_id, _params, _socket) do
    {:error, %{reason: "unauthorized"}}
  end

  def handle_info(%Event{body: %Transport{dest: dest, body: %Room.Message{source: source, dest: _dest, room_id: room_id, text: text}}}, socket) do
    IO.puts("LoungeChannel[#{dest}] <= Transport(Room.Message): room=#{room_id}")
    push socket, "room:message", %{source: source, room_id: room_id, body: text}
    {:noreply, socket}
  end

  def handle_info(%Event{body: %Transport{dest: dest, body: %Room.Setting{source: _source, room_id: room_id, name: name, color: color}}}, socket) do
    IO.puts("LoungeChannel[#{dest}] <= Transport(Room.Setting): room=#{room_id}")
    push socket, "room:setting", %{room_id: room_id, name: name, color: color}
    {:noreply, socket}
  end

  def handle_in("room:create", %{"source" => source}, socket) do
    IO.puts("Channel#room:create: by #{source}")
    %{body: body} = handle fn id ->
      perform id, %Request{body: %Room.Create{source: source}}
    end
    {:reply, {:ok, body}, socket}
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
        body: %Transport{
          dest: source,
          direction: :incoming,
          body: %Room.Message{
            source: source,
            dest: "*",
            room_id: room_id,
            text: body
          }
        }
      }
    end
    {:reply, :ok, socket}
  end

  def handle_in("room:setting", %{"source" => source, "room_id" => room_id, "name" => name, "color" => color}, socket) do
    IO.puts("Channel#room:setting: to=#{room_id}, name=#{name}, color=#{color}, by=#{source}")
    handle fn id ->
      perform id, %Dispatch{
        body: %Transport{
          dest: source,
          direction: :incoming,
          body: %Room.Setting{
            source: source,
            room_id: room_id,
            name: name,
            color: color
          }
        }
      }
    end
    {:noreply, socket}
  end

  def handle_in("avatar:profile", %{"source" => source, "name" => name}, socket) do
    IO.puts("Channel#avatar:profile: name=#{name}, by=#{source}")
    handle fn id ->
      perform id, %Dispatch{
        body: %Transport{
          dest: source,
          direction: :incoming,
          body: %Avatar.Profile{
            source: source,
            name: name
          }
        }
      }
    end
    {:noreply, socket}
  end
end

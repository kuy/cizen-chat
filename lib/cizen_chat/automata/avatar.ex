alias Cizen.Effects.{Receive, Request, Subscribe, Start, Dispatch}
alias Cizen.{Event, Filter}
alias CizenChat.Automata
alias CizenChat.Events.{Transport, Room}

defmodule CizenChat.Automata.Avatar do
  use Cizen.Automaton

  defstruct []

  @impl true
  def spawn(id, _) do
    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.Create{source: source_id}} ->
          source_id == id
        end
      )
    }

    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.Enter{source: source_id}} ->
          source_id == id
        end
      )
    }

    # Transport from Phoenix Channel
    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Transport{dest: dest, direction: dir, body: %Room.Message{source: source}}} ->
          source == id and dest == id and dir == :incoming
        end
      )
    }

    # Message from Room
    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.Message{dest: dest_id}} ->
          dest_id == id
        end
      )
    }

    # Setting from Room
    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.Setting{}} -> true end
      )
    }

    name = id |> String.split("-") |> List.first
    %{
      name: name, # avatar name
      rooms: []   # entered rooms
    }
  end

  @impl true
  def yield(id, state) do
    event = perform id, %Receive{}
    case event.body do
      %Room.Create{source: _source} ->
        IO.puts("Avatar[#{state.name}] <= Room.Create")
        room_id = perform id, %Start{saga: %Automata.Room{created_by: id}}

        %{body: %{name: name, color: color}} = perform id, %Request{
          body: %Room.SelfIntro{room_id: room_id}
        }

        IO.puts("Avatar[#{state.name}] => Room.Create.Done: room_id=#{room_id}")
        perform id, %Dispatch{
          body: %Room.Create.Done{
            create_id: event.id,
            room_id: room_id,
            name: name,
            color: color
          }
        }

        %{
          name: state.name,
          rooms: [room_id | state.rooms]
        }
      %Room.Enter{source: _source, room_id: room_id} ->
        IO.puts("Avatar[#{state.name}] <= Room.Enter: room_id=#{room_id}")
        %{
          name: state.name,
          rooms: [room_id | state.rooms]
        }

      # Phoenix Channel -> Gateway -> Avatar(me) -> Room
      %Transport{dest: _dest, direction: _direction, body: body} ->
        case body do
          %Room.Message{source: _source, dest: _dest, room_id: room_id, text: text} ->
            IO.puts("Avatar[#{state.name}] <= Transport(Room.Message): '#{text}' by me at #{room_id}")
            perform id, %Dispatch{body: body}
            state
          _ -> state
        end

      # Room -> Avatar(me) -> Gateway -> Phoenix Channel
      %Room.Message{source: source, dest: dest, room_id: room_id, text: text} ->
        IO.puts("Avatar[#{state.name}] <= Room.Message: '#{text}' by #{source} at #{room_id}")
        perform id, %Dispatch{
          body: %Transport{
            dest: id, # me
            direction: :outgoing,
            body: %Room.Message{
              source: source,
              dest: dest,
              room_id: room_id,
              text: text
            }
          }
        }
        state
      %Room.Setting{source: source, room_id: room_id, name: name, color: color} ->
        if source != id do
          IO.puts("Avatar[#{state.name}] <= Room.Setting: name=#{name}, color=#{color}")
          perform id, %Dispatch{
            body: %Transport{
              dest: id, # me
              direction: :outgoing,
              body: %Room.Setting{
                source: source,
                room_id: room_id,
                name: name,
                color: color
              }
            }
          }
        end
        state
    end
  end
end

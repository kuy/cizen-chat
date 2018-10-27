alias Cizen.Effects.{Receive, Subscribe, Start, Dispatch}
alias Cizen.EventFilter
alias CizenChat.Automata
alias CizenChat.Events.Room

defmodule CizenChat.Automata.Avatar do
  use Cizen.Automaton

  defstruct []

  @impl true
  def spawn(id, _) do
    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Create,
        event_body_filters: [
          %Room.SourceFilter{value: id}
        ]
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Enter,
        event_body_filters: [
          %Room.SourceFilter{value: id}
        ]
      )
    }

    # Transport Message from Phoenix Channel
    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Message.Transport,
        event_body_filters: [
          %Room.SourceFilter{value: id},
          %Room.DestFilter{value: id},
          %Room.DirectionFilter{value: :incoming}
        ]
      )
    }

    # Message from Room
    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Message,
        event_body_filters: [
          %Room.DestFilter{value: id}
        ]
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
    # IO.puts("Avatar: name=#{state.name}")
    event = perform id, %Receive{}
    case event.body do
      %Room.Create{source: _source} ->
        IO.puts("Avatar[#{state.name}] <= Room.Create")
        room_id = perform id, %Start{saga: %Automata.Room{created_by: id}}

        IO.puts("Avatar[#{state.name}] => Room.Create.Done: room_id=#{room_id}")
        perform id, %Dispatch{
          body: %Room.Create.Done{
            create_id: event.id,
            room_id: room_id
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
      %Room.Message.Transport{source: source, dest: _dest, direction: _direction, room_id: room_id, text: text} ->
        IO.puts("Avatar[#{state.name}] <= Room.Message.Transport: '#{text}' at #{room_id}")
        perform id, %Dispatch{
          body: %Room.Message{
            source: source,
            dest: "*",
            room_id: room_id,
            text: text
          }
        }
        state
      %Room.Message{source: source, dest: dest, room_id: room_id, text: text} ->
        IO.puts("Avatar[#{state.name}] <= Room.Message: '#{text}' by #{source} at #{room_id}")
        perform id, %Dispatch{
          body: %Room.Message.Transport{
            source: source,
            dest: dest,
            direction: :outgoing,
            room_id: room_id,
            text: text
          }
        }
        state
    end
  end
end

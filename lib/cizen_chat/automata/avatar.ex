alias Cizen.Effects.{Receive, Request, Subscribe, Start, Dispatch}
alias Cizen.{Event, Filter}
alias CizenChat.Automata
alias CizenChat.Events.{Transport, Avatar, Room}

defmodule CizenChat.Automata.Avatar do
  use Cizen.Automaton

  defstruct []

  @impl true
  def spawn(id, _) do
    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Avatar.SelfIntro{avatar_id: avatar_id}} ->
          avatar_id == id
        end
      )
    }

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
        fn %Event{body: %Transport{dest: dest, direction: dir, body: %Avatar.Profile{source: source}}} ->
          dest == id and dir == :incoming and source == id
        end
      )
    }
    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Transport{dest: dest, direction: dir, body: %Room.Message{source: source}}} ->
          dest == id and dir == :incoming and source == id
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

    # Request advertisement to all rooms
    perform id, %Dispatch{
      body: %Room.Advertise{joiner_id: id}
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
      %Avatar.SelfIntro{avatar_id: _avatar_id} ->
        IO.puts("Avatar[#{state.name}] <= Avatar.SelfIntro")
        perform id, %Dispatch{
          body: %Avatar.SelfIntro.Explain{
            self_intro_id: event.id,
            name: state.name
          }
        }
        state
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
          %Avatar.Profile{source: _source, name: name} ->
            IO.puts("Avatar[#{state.name}] <= Transport(Avatar.Profile): Rename '#{state.name}' -> '#{name}'")
            put_in(state, [:name], name)
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

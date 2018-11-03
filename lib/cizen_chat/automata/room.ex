alias Cizen.Effects.{Receive, Subscribe, Dispatch}
alias Cizen.{Event, Filter}
alias CizenChat.Events.{Transport, Room}

defmodule CizenChat.Automata.Room do
  use Cizen.Automaton

  defstruct [:created_by]

  @impl true
  def spawn(id, %__MODULE__{created_by: created_by}) do
    IO.puts("Room: created by #{created_by}")

    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Transport{direction: dir, body: %Room.Setting{room_id: room_id}}} ->
          dir == :incoming and room_id == id
        end
      )
    }

    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.Enter{room_id: room_id}} ->
          room_id == id
        end
      )
    }

    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.Message{dest: dest_id, room_id: room_id}} ->
          # FIXME: "*" is ugly workaround
          dest_id == "*" and room_id == id
        end
      )
    }

    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.Advertise{}} -> true end
      )
    }

    perform id, %Subscribe{
      event_filter: Filter.new(
        fn %Event{body: %Room.SelfIntro{room_id: room_id}} ->
          room_id == id
        end
      )
    }

    # Advertise other avatars
    name = id |> String.split("-") |> List.first
    color = :green
    perform id, %Dispatch{
      body: %Room.Setting{
        source: created_by,
        room_id: id,
        name: name,
        color: color
      }
    }

    %{
      name: name,
      color: color,          # theme color of this room
      members: [created_by], # list of avatar ids
      created_by: created_by # creator of this room
    }
  end

  @impl true
  def yield(id, state) do
    event = perform id, %Receive{}
    case event.body do
      %Transport{dest: _dest, direction: _direction, body: body} ->
        case body do
          %Room.Setting{source: source, room_id: room_id, name: name, color: color} ->
            IO.puts("Room[#{state.name}] <= Transport(Room.Setting)")
            perform id, %Dispatch{
              body: %Room.Setting{
                source: source,
                room_id: room_id,
                name: name,
                color: color
              }
            }
            state
            |> put_in([:name], name)
            |> put_in([:color], color)
          _ -> state
        end
      %Room.Advertise{joiner_id: joiner_id} ->
        IO.puts("Room[#{state.name}] <= Room.Advertise: joiner=#{joiner_id}")
        perform id, %Dispatch{
          body: %Transport{
            dest: joiner_id,
            direction: :outgoing,
            body: %Room.Setting{
              source: joiner_id,
              room_id: id,
              name: state.name,
              color: state.color
            }
          }
        }
        state
      %Room.SelfIntro{room_id: _room_id} ->
        IO.puts("Room[#{state.name}] <= Room.SelfIntro")
        perform id, %Dispatch{
          body: %Room.SelfIntro.Explain{
            self_intro_id: event.id,
            name: state.name,
            color: state.color
          }
        }
        state
      %Room.Enter{source: source, room_id: _room_id} ->
        IO.puts("Room[#{state.name}] <= Room.Enter: source=#{source}")
        put_in(state, [:members], [source | state.members])
      %Room.Message{source: source, dest: _dest, room_id: _room_id, text: text} ->
        IO.puts("Room[#{state.name}] <= Room.Message: text=#{text}, source=#{source}")

        # Broadcast event to room members except source
        state.members
        |> Enum.filter(fn avatar_id -> avatar_id != source end)
        |> Enum.each(fn avatar_id ->
          perform id, %Dispatch{
            body: %Room.Message{
              source: source,
              dest: avatar_id,
              room_id: id,
              text: text
            }
          }
        end)
        state
    end
  end
end

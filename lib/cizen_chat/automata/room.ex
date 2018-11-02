alias Cizen.Effects.{Receive, Subscribe, Dispatch}
alias Cizen.EventFilter
alias CizenChat.Events
alias CizenChat.Events.{Transport, Room}

defmodule CizenChat.Automata.Room do
  use Cizen.Automaton

  defstruct [:created_by]

  @impl true
  def spawn(id, %__MODULE__{created_by: created_by}) do
    IO.puts("Room: created by #{created_by}")

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Transport,
        event_body_filters: [
          %Events.DirectionFilter{value: :incoming},
          %Transport.RoomIDFilter{value: id}
        ]
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Enter,
        event_body_filters: [
          %Events.RoomIDFilter{value: id}
        ]
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Message,
        event_body_filters: [
          %Events.DestFilter{value: "*"},
          %Events.RoomIDFilter{value: id}
        ]
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Advertise
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.SelfIntro,
        event_body_filters: [
          %Events.RoomIDFilter{value: id}
        ]
      )
    }

    %{
      name: id |> String.split("-") |> List.first,
      color: :green,         # theme color of this room
      members: [created_by], # list of avatar ids
      created_by: created_by # creator of this room
    }
  end

  @impl true
  def yield(id, state) do
    IO.puts("Room[#{state.name}]: members=#{Enum.join(state.members, ", ")}")
    event = perform id, %Receive{}
    case event.body do
      %Transport{source: _source, dest: _dest, direction: _direction, body: body} ->
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
            source: joiner_id,
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

alias Cizen.Effects.{Receive, Subscribe, Dispatch}
alias Cizen.EventFilter
alias CizenChat.Events.Room

defmodule CizenChat.Automata.Room do
  use Cizen.Automaton

  defstruct [:created_by]

  @impl true
  def spawn(id, %__MODULE__{created_by: created_by}) do
    IO.puts("Room: created by #{created_by}")

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Enter,
        event_body_filters: [
          %Room.RoomIDFilter{value: id}
        ]
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Message,
        event_body_filters: [
          %Room.DestFilter{value: "*"},
          %Room.RoomIDFilter{value: id}
        ]
      )
    }

    %{
      created_by: created_by, # creator of this room
      members: [created_by]   # list of avatar ids
    }
  end

  @impl true
  def yield(id, state) do
    IO.puts("Room[#{id}]: members=#{Enum.join(state.members, ", ")}")
    event = perform id, %Receive{}
    case event.body do
      %Room.Enter{source: source, room_id: _room_id} ->
        IO.puts("Room[#{id}] <= Room.Enter: source=#{source}")
        %{
          created_by: state.created_by,
          members: [source | state.members]
        }
      %Room.Message{source: source, dest: _dest, room_id: _room_id, text: text} ->
        IO.puts("Room[#{id}] <= Room.Message: text=#{text}, source=#{source}")

        # Broadcast to room members except source
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

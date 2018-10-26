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
          %Room.AvatarIDFilter{value: id}
        ]
      )
    }

    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Room.Message,
        event_body_filters: [
          %Room.AvatarIDFilter{value: id}
        ]
      )
    }

    %{
      name: "Avatar #{id}", # avatar name
      rooms: []             # entered rooms
    }
  end

  @impl true
  def yield(id, state) do
    IO.puts("Avatar: name=#{state.name}")
    event = perform id, %Receive{}
    case event.body do
      %Room.Create{avatar_id: _avatar_id} ->
        IO.puts("Avatar <= Room.Create: from #{state.name}")
        room_id = perform id, %Start{saga: %Automata.Room{created_by: id}}

        IO.puts("Avatar => Room.Create.Done: by #{state.name} room_id=#{room_id}")
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
      %Room.Message{avatar_id: _avatar_id, room_id: room_id, text: text} ->
        IO.puts("Avatar <= Room.Message: '#{text}' from #{state.name} at #{room_id}")
        state
    end
  end
end

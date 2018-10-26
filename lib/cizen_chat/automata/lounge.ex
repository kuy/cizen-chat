alias Cizen.Effects.{Start, Receive, Subscribe, Dispatch}
alias Cizen.EventFilter
alias CizenChat.Events.Lounge
alias CizenChat.Automata

defmodule CizenChat.Automata.Lounge do
  use Cizen.Automaton

  defstruct []

  @impl true
  def spawn(id, _) do
    perform id, %Subscribe{
      event_filter: EventFilter.new(event_type: Lounge.Join)
    }

    %{
      avatars: [], # list of avatar IDs
      rooms: []    # list of room IDs
    }
  end

  @impl true
  def yield(id, state) do
    IO.puts("Lounge: Avatars=#{Enum.join(state.avatars, ", ")}")
    IO.puts("Lounge: Rooms=#{Enum.join(state.rooms, ", ")}")

    event = perform id, %Receive{}
    case event.body do
      %Lounge.Join{} ->
        IO.puts("Lounge <= Join")
        avatar_id = perform id, %Start{saga: %Automata.Avatar{}}

        IO.puts("Lounge <= Join: avatar_id=#{avatar_id}")
        perform id, %Dispatch{
          body: %Lounge.Join.Welcome{
            join_id: event.id,
            avatar_id: avatar_id
          }
        }

        %{
          avatars: [avatar_id | state.avatars],
          rooms: state.rooms
        }
    end
  end
end

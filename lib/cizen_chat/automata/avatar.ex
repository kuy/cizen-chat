alias Cizen.Effects.{Receive, Subscribe}
alias Cizen.EventFilter
alias CizenChat.Events

defmodule CizenChat.Automata.Avatar do
  use Cizen.Automaton

  defstruct []

  @impl true
  def spawn(id, _) do
    perform id, %Subscribe{
      event_filter: EventFilter.new(
        event_type: Events.Message,
        event_body_filters: [
          %Events.Message.AvatarIDFilter{value: id}
        ]
      )
    }

    %{
      name: "Avatar #{id}"
    }
  end

  @impl true
  def yield(id, state) do
    IO.puts("Avatar: name=#{state.name}")
    event = perform id, %Receive{}
    case event.body do
      %Events.Message{avatar_id: avatar_id, text: text} ->
        IO.puts("Avatar <= Message: '#{text}' from #{state.name} (#{avatar_id})")
        state
    end
  end
end

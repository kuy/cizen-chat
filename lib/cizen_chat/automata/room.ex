defmodule CizenChat.Automata.Room do
  use Cizen.Automaton

  defstruct [:created_by]

  @impl true
  def spawn(_, %__MODULE__{created_by: created_by}) do
    IO.puts("Room: created by #{created_by}")
    %{created_by: created_by}
  end

  @impl true
  def yield(id, state) do
    IO.puts("Tick: #{id}")
    Process.sleep(5000)
    state
  end
end

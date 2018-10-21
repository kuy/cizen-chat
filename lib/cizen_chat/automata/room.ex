defmodule CizenChat.Automata.Room do
  use Cizen.Automaton

  defstruct []

  @impl true
  def spawn(_, _) do
    :loop
  end

  @impl true
  def yield(id, :loop) do
    IO.puts("Tick")
    Process.sleep(5000)
    :loop
  end
end

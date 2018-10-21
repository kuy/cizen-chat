alias Cizen.Effects.{Dispatch, Request}
alias CizenChat.Events

defmodule CizenChatWeb.LoungeChannel do
  use Phoenix.Channel
  use Cizen.Effectful

  def join("lounge:hello", _message, socket) do
    avatar_id = handle fn id ->
      welcome_event = perform id, %Request{body: %Events.Join{}}
      welcome_event.body.avatar_id
    end
    {:ok, %{id: avatar_id}, socket}
  end

  def join("lounge:" <> _private_room_id, _params, _socket) do
    {:error, %{reason: "unauthorized"}}
  end

  def handle_in("room:message", %{"avatar_id" => avatar_id, "body" => body}, socket) do
    IO.puts("Channel.Message: #{body} from #{avatar_id}")
    handle fn id ->
      perform id, %Dispatch{
        body: %Events.Message{avatar_id: avatar_id, text: body}
      }
    end
    {:reply, :ok, socket}
  end
end

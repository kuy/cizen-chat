# This file is responsible for configuring your application
# and its dependencies with the aid of the Mix.Config module.
#
# This configuration file is loaded before any dependency and
# is restricted to this project.
use Mix.Config

# Configures the endpoint
config :cizen_chat, CizenChatWeb.Endpoint,
  url: [host: "localhost"],
  secret_key_base: "XMSrsBbK+ks8n2u9d6JjD/Kflf5nWF2JcYGNAHGr+rR5v1eCL5wVYWPf2c+C18Js",
  render_errors: [view: CizenChatWeb.ErrorView, accepts: ~w(html json)],
  pubsub: [name: CizenChat.PubSub,
           adapter: Phoenix.PubSub.PG2]

# Configures Elixir's Logger
config :logger, :console,
  format: "$time $metadata[$level] $message\n",
  metadata: [:user_id]

# Import environment specific config. This must remain at the bottom
# of this file so it overrides the configuration defined above.
import_config "#{Mix.env}.exs"

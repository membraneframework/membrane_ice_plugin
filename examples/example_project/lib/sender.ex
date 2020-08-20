defmodule Example.Sender do
  use Membrane.Pipeline

  alias Membrane.Element.File

  require Membrane.Logger

  @impl true
  def handle_init(_) do
    children = %{
      source: %File.Source{
        location: "~/Videos/test-video.h264"
      },
      address_provider: Example.AddressProvider,
      sink: Membrane.ICE.Sink
    }

    links = [
      link(:source) |> to(:address_provider) |> to(:sink)
    ]

    spec = %ParentSpec{
      children: children,
      links: links
    }

    {{:ok, spec: spec}, %{}}
  end

  @impl true
  def handle_stopped_to_prepared(_ctx, state) do
    n_components = 1
    {{:ok, forward: {:sink, {:add_stream, n_components}}}, state}
  end

  @impl true
  def handle_notification({:stream_id, stream_id} = msg, _from, _ctx, state) do
    Membrane.Logger.debug("#{inspect(msg)}")
    state = Map.put(state, :stream_id, stream_id)
    {{:ok, forward: {:sink, {:gather_candidates, stream_id}}}, state}
  end

  @impl true
  def handle_notification({:new_candidate_full, _candidate} = msg, _from, _ctx, state) do
    Membrane.Logger.debug("#{inspect(msg)}")
    {:ok, state}
  end

  @impl true
  def handle_notification(:gathering_done = msg, _from, _ctx, state) do
    Membrane.Logger.debug("#{inspect(msg)}")
    {{:ok, forward: {:sink, {:get_local_credentials, state.stream_id}}}, state}
  end

  @impl true
  def handle_notification({:local_credentials, _credentials} = msg, _from, _ctx, state) do
    Membrane.Logger.debug("#{inspect(msg)}")
    {:ok, state}
  end

  @impl true
  def handle_other({:set_remote_credentials, remote_credentials, stream_id} = msg, _ctx, state) do
    Membrane.Logger.debug("#{inspect(msg)}")
    {{:ok, forward: {:sink, {:set_remote_credentials, remote_credentials, stream_id}}}, state}
  end

  @impl true
  def handle_other({:set_remote_candidate, candidate} = msg, _ctx, state) do
    Membrane.Logger.debug("#{inspect(msg)}")
    {{:ok, forward: {:sink, {:set_remote_candidate, candidate, state.stream_id, 1}}}, state}
  end
end
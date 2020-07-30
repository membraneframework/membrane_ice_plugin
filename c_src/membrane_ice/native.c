#include "native.h"

#include <gio/gnetworking.h>
#include <nice/agent.h>
#include <stdio.h>

static void cb_candidate_gathering_done(NiceAgent *, guint, gpointer);
static void cb_component_state_changed(NiceAgent *, guint, guint, guint,
                                       gpointer);
static void cb_new_selected_pair(NiceAgent *, guint, guint, gchar *, gchar *,
                                 gpointer);
static void cb_recv(NiceAgent *, guint, guint, guint, gchar *, gpointer);

static GMainLoop *gloop;
static UnifexEnv *env;

// TODO handle errors

UNIFEX_TERM init(UnifexEnv *envl) {
  State *state = unifex_alloc_state(envl);
  state->gloop = g_main_loop_new(NULL, FALSE);
  state->agent = nice_agent_new(g_main_loop_get_context(state->gloop), NICE_COMPATIBILITY_RFC5245);
  gloop = state->gloop;
  NiceAgent *agent = state->agent;
  g_object_set(agent, "stun-server", "64.233.161.127", NULL);
  g_object_set(agent, "stun-server-port", 19302, NULL);
  g_object_set(agent, "controlling-mode", FALSE, NULL);

  g_signal_connect(G_OBJECT(agent), "candidate-gathering-done",
                   G_CALLBACK(cb_candidate_gathering_done), NULL);
  g_signal_connect(G_OBJECT(agent), "component-state-changed",
                   G_CALLBACK(cb_component_state_changed), NULL);
  g_signal_connect(G_OBJECT(agent), "new-selected-pair",
                   G_CALLBACK(cb_new_selected_pair), NULL);

   guint stream_id = nice_agent_add_stream(agent, 1);

  nice_agent_attach_recv(agent, stream_id, 1, g_main_loop_get_context(state->gloop),
                         cb_recv, NULL);

  state->stream_id = stream_id;

  env = envl;
  return init_result_ok(env, state);
}

static void cb_candidate_gathering_done(NiceAgent *agent, guint stream_id,
                                        gpointer user_data) {
  UNIFEX_UNUSED(user_data);
  gchar ipstr[INET6_ADDRSTRLEN];
  GSList *cands = nice_agent_get_local_candidates(agent, stream_id, 1);
  for (GSList *cand = cands; cand != NULL; cand = cand->next)
  {
    NiceCandidate *c = (NiceCandidate *)cand->data;
    nice_address_to_string(&c->addr, ipstr);
    send_candidate(env, *env->reply_to, 0, ipstr);
  }
  send_gathering_done(env, *env->reply_to, 0);
  g_main_loop_quit(gloop);
}

static void cb_component_state_changed(NiceAgent *agent, guint stream_id,
                                       guint component_id, guint state,
                                       gpointer user_data) {
  UNIFEX_UNUSED(agent);
  UNIFEX_UNUSED(stream_id);
  UNIFEX_UNUSED(component_id);
  UNIFEX_UNUSED(state);
  UNIFEX_UNUSED(user_data);
}

static void cb_new_selected_pair(NiceAgent *agent, guint stream_id,
                                 guint component_id, gchar *lfoundation,
                                 gchar *rfoundation, gpointer user_data) {
   UNIFEX_UNUSED(agent);
   UNIFEX_UNUSED(stream_id);
   UNIFEX_UNUSED(component_id);
   UNIFEX_UNUSED(lfoundation);
   UNIFEX_UNUSED(rfoundation);
   UNIFEX_UNUSED(user_data);
 }

static void cb_recv(NiceAgent *agent, guint stream_id, guint component_id,
                    guint len, gchar *buf, gpointer user_data) {
  UNIFEX_UNUSED(agent);
  UNIFEX_UNUSED(stream_id);
  UNIFEX_UNUSED(component_id);
  UNIFEX_UNUSED(len);
  UNIFEX_UNUSED(buf);
  UNIFEX_UNUSED(user_data);
  if (len == 1 && buf[0] == '\0')
    g_main_loop_quit(gloop);
  printf("%.*s", len, buf);
  fflush(stdout);
}

UNIFEX_TERM start_gathering_candidates(UnifexEnv *_env, State *state) {
  UNIFEX_UNUSED(_env);
  g_networking_init();
  nice_agent_gather_candidates(state->agent, state->stream_id);
  g_main_loop_run(gloop);
  return start_gathering_candidates_result_ok(env, state);
}

void handle_destroy_state(UnifexEnv *env, State *state) {
  UNIFEX_UNUSED(env);
  if (state->gloop) {
    g_main_loop_unref(state->gloop);
    state->gloop = NULL;
  }
  if (state->agent) {
    g_object_unref(state->agent);
    state->agent = NULL;
  }
}

# MCP server (planned)

Status: **not implemented in this revision.** This page describes the interface the build order
specifies so that the AI layer can be built against it later.

## Transport

- JSON-RPC 2.0 over `127.0.0.1:<port>`, `POST /`.
- Each request carries a bearer token in the `X-SF-Token` header. The token is written to
  `config.json` in the configuration directory.
- `shaderforge_mcp --port N` will bridge stdio to that TCP endpoint so that MCP clients can
  launch it as a command.

## Tools (exact names)

`get_contract`, `get_current_shader`, `set_shader`, `compile`, `set_uniform`, `list_presets`,
`save_preset`, `render_thumbnail`, `get_audio_state`.

`get_contract` returns the text of `docs/SHADER_CONTRACT.md`, so a client can ground its prompts
in the same rules that the validator enforces.

## Client configuration

Not available yet. A config snippet will be added once the launcher exists.

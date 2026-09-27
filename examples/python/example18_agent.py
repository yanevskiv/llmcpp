#!/usr/bin/env python3
"""Adapt a chat-completions server to llmcpp's version 1 stdio protocol.

The server must support OpenAI-style tools and tool-call results. Configure
its base_url and served model name in examples/json/example18_config.json.
This example uses only the Python standard library and can be adapted to a
different server API.
"""

import json
import os
import sys
import time
import urllib.request


class CompilerConnection:
    """Exchange JSON-RPC messages with the compiler over standard streams."""

    def __init__(self):
        self.next_id = 0

    # Send a single newline-delimited message.
    def send(self, message):
        print(json.dumps(message), flush=True)

    # Read a message or stop when the compiler closes the connection.
    def receive(self):
        line = sys.stdin.readline()
        if not line:
            raise EOFError("compiler closed the connection")
        return json.loads(line)

    # Request a compiler tool or MCP handshake operation.
    def request(self, method, params):
        self.next_id += 1
        request_id = "example-%d" % self.next_id
        self.send({
            "jsonrpc": "2.0",
            "id": request_id,
            "method": method,
            "params": params,
        })
        while True:
            reply = self.receive()
            if reply.get("id") == request_id and "method" not in reply:
                if "error" in reply:
                    raise RuntimeError(reply["error"]["message"])
                return reply["result"]


# Translate MCP tool definitions into chat-completions tools.
def chat_tools(tools):
    return [
        {
            "type": "function",
            "function": {
                "name": tool["name"],
                "description": tool["description"],
                "parameters": tool["inputSchema"],
            },
        }
        for tool in tools
    ]


# Generate one body, returning only after an accepted submit or a failure.
def generate(connection, tools, task):
    if task.get("protocol_version") != 1:
        raise ValueError("unsupported llmcpp protocol version")
    config = task.get("agent_config", {})
    model = task["generation"].get("model") or config.get("model")
    if not model:
        raise ValueError("configure a served model name")
    base_url = config["base_url"].rstrip("/")
    api_key = os.environ.get(config.get("api_key_env", "LOCAL_MODEL_API_KEY"), "")
    headers = {"Content-Type": "application/json"}
    if api_key:
        headers["Authorization"] = "Bearer " + api_key
    messages = [
        {
            "role": "system",
            "content": task["system_prompt"],
        },
        {
            "role": "user",
            "content": (
                json.dumps(task["generation"])
                + "\nGenerate " + task["name"] + ". Start with get_task."
            ),
        },
    ]
    deadline = time.monotonic() + task["limits"]["timeout_seconds"]
    for _ in range(task["limits"]["max_tool_calls"] + 1):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("generation deadline reached")
        body = {
            "model": model,
            "messages": messages,
            "tools": chat_tools(tools),
        }
        request = urllib.request.Request(
            base_url + "/chat/completions",
            data=json.dumps(body).encode(),
            headers=headers,
        )
        with urllib.request.urlopen(request, timeout=remaining) as response:
            reply = json.load(response)
        message = reply["choices"][0]["message"]
        calls = message.get("tool_calls", [])
        if not calls:
            raise RuntimeError("model stopped without submitting a body")
        messages.append(message)
        for call in calls:
            function = call["function"]
            result = connection.request("tools/call", {
                "name": function["name"],
                "arguments": json.loads(function["arguments"]),
            })
            text = "\n".join(part.get("text", "") for part in result.get("content", []))
            messages.append({
                "role": "tool",
                "tool_call_id": call["id"],
                "content": text,
            })
            if function["name"] == "submit" and not result.get("isError"):
                return {
                    "status": "ok",
                    "model": reply.get("model", model),
                }
    raise RuntimeError("generation tool budget exhausted")


# Serve compiler generation requests using the configured model server.
def main():
    connection = CompilerConnection()
    connection.request("initialize", {
        "protocolVersion": "2025-06-18",
        "capabilities": {},
        "clientInfo": {
            "name": "example18-agent",
            "version": "1",
        },
    })
    tools = connection.request("tools/list", {})["tools"]
    connection.send({
        "jsonrpc": "2.0",
        "method": "notifications/initialized",
    })
    while True:
        message = connection.receive()
        if message.get("method") != "llm/generate":
            if "id" in message:
                connection.send({
                    "jsonrpc": "2.0",
                    "id": message["id"],
                    "error": {
                        "code": -32601,
                        "message": "method not found",
                    },
                })
            continue
        try:
            result = generate(connection, tools, message["params"])
        except Exception as error:
            result = {
                "status": "error",
                "message": str(error),
            }
        connection.send({
            "jsonrpc": "2.0",
            "id": message["id"],
            "result": result,
        })


if __name__ == "__main__":
    try:
        main()
    except (EOFError, BrokenPipeError, KeyboardInterrupt):
        pass

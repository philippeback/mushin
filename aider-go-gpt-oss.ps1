$env:OPENAI_API_BASE = "http://192.168.87.31:1234/v1"
$env:OPENAI_API_KEY = "dummy"

aider --dark-mode --chat-language en --model openai/gpt-oss-120b  --show-diffs --no-show-model-warnings --no-lint




module.exports = {
  "disableEmoji": false,
  "list": [
    "feat",
    "fix",
	"test",
    "chore",
    "docs",
    "refactor",
    "style",
    "ci",
    "perf"
  ],
  "maxMessageLength": 64,
  "minMessageLength": 3,
  "questions": [
    "type",
    "scope",
    "subject",
    "body",
    "breaking",
    "issues",
    "lerna"
  ],
  "scopes": [],
  "types": {
    "chore": {
      "description": "构建脚本相关的改变",
      "emoji": "🤖",
      "value": "chore"
    },
    "ci": {
      "description": "系统集成相关的改变，如cpack,install",
      "emoji": "🎡",
      "value": "ci"
    },
    "docs": {
      "description": "文档相关的改变",
      "emoji": "✏️",
      "value": "docs"
    },
    "feat": {
      "description": "添加一个新功能",
      "emoji": "🎸",
      "value": "feat"
    },
    "fix": {
      "description": "修复一个BUG",
      "emoji": "🐛",
      "value": "fix"
    },
    "perf": {
      "description": "改善性能",
      "emoji": "⚡️",
      "value": "perf"
    },
    "refactor": {
      "description": "代码重构",
      "emoji": "💡",
      "value": "refactor"
    },
    "release": {
      "description": "版本发布",
      "emoji": "🏹",
      "value": "release"
    },
    "style": {
      "description": "Markup, white-space, formatting, missing semi-colons...",
      "emoji": "💄",
      "value": "style"
    },
    "test": {
      "description": "添加测试用例",
      "emoji": "💍",
      "value": "test"
    }
  }
};
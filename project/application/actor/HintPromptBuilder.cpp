#include "HintPromptBuilder.h"

std::string HintPromptBuilder::BuildPrompt(const HintLog& hintLog) {
	std::string prompt;

	prompt += "あなたはゲーム攻略を支援するAIです。\n";
	prompt += "プレイヤーの行動ログを分析して、その人専用のヒントを生成してください。\n\n";

	prompt += "重要なルール:\n";
	prompt += "・正解は知っていますが、絶対に直接言わないでください。\n";
	prompt += "・「正解は3番です」のようなネタバレは禁止です。\n";
	prompt += "・固定文ではなく、ログの傾向を見て自然なヒントを出してください。\n";
	prompt += "・短く、ゲーム中に表示しやすい一文にしてください。\n\n";

	prompt += "ステージ情報:\n";
	prompt += "・stageId: five_road_001\n";
	prompt += "・correctIndex: 2\n\n";

	prompt += "プレイヤー情報:\n";
	prompt += "・missCount: " + std::to_string(hintLog.GetMissCount()) + "\n";
	prompt += "・correctCount: " + std::to_string(hintLog.GetCorrectCount()) + "\n";
	prompt += "・totalSelectCount: " + std::to_string(hintLog.GetTotalSelectCount()) + "\n\n";

	prompt += "選択ログ:\n";

	const auto& logs = hintLog.GetLogs();

	for (size_t i = 0; i < logs.size(); i++) {
		prompt += std::to_string(i + 1);
		prompt += "回目: 択";
		prompt += std::to_string(logs[i].roadIndex + 1);
		prompt += " → ";
		prompt += logs[i].isCorrect ? "正解" : "はずれ";
		prompt += "\n";
	}

	prompt += "\n";
	prompt += "この情報をもとに、プレイヤーに次の行動のヒントを1文で返してください。\n";

	return prompt;
}
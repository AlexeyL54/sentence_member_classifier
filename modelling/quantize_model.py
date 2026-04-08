from onnxruntime.quantization import quantize_dynamic, QuantType

model_path = "models/bert_ner_model.onnx"
quantized_model_path = model_path.replace(".onnx", "_quantized.onnx")

# Динамическая квантация
quantize_dynamic(
    model_input=model_path,
    model_output=quantized_model_path,
    weight_type=QuantType.QUInt8,  # Можно попробовать QInt8
)

print(f"✓ Квантованная модель сохранена в: {quantized_model_path}")

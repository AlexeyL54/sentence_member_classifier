def generate_dataset(input_file="deeprich_oborot.txt", output_file="newtext.txt"):
    """
    Генерация датасета из текстового файла в формате JSON-like структуры
    
    Args:
        input_file (str): Путь к входному файлу с текстом
        output_file (str): Путь к выходному файлу для сохранения датасета
    
    Returns:
        bool: True если успешно, False в случае ошибки
    """
    try:
        with open(input_file, "r", encoding="utf-8") as file:
            with open(output_file, 'w') as outfile:
                outfile.write('[\n')
                k=0
                while True:
                    line = file.readline()
                    if not line:
                        break
                    
                    if len(line) > 1:
                        if(k!=0):
                            outfile.write(',\n')
                        else:
                            outfile.write('\n')
                        k+=1
                        outfile.write('\t{\n')
                        outfile.write('\t\t"tokens": [')
                        
                        # Убираем символ новой строки
                        line = line.strip()
                        lines = line.split(' ')
                        
                        # Записываем токены
                        for idx, word in enumerate(lines):
                            if idx != 0:
                                outfile.write(', ')
                            outfile.write('"' + word + '"')
                        
                        outfile.write("],")
                        outfile.write('\n\t\t\t"tags": [ ')
                        
                        # Записываем пустые теги (для последующего заполнения)
                        for l in range(len(lines) - 1):
                            outfile.write('"", ')
                        outfile.write('""]')
                        outfile.write('\n\t}')
                
                outfile.write('\n]')
        
        print(f" Dataset successfully generated from '{input_file}' to '{output_file}'")
        return True
        
    except FileNotFoundError:
        print(f" Error: Input file '{input_file}' not found")
        return False
    except Exception as e:
        print(f" Error generating dataset: {e}")
        return False
 

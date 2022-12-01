with open("logo1.txt", "r") as FILE:
    lines1 = [line[:-1] for line in FILE.readlines()]

with open("logo2.txt", "r") as FILE:
    lines2 = [line[:-1] for line in FILE.readlines()]

with open("logo3.txt", "r") as FILE:
    lines3 = [line[:-1] for line in FILE.readlines()]

with open("logo4.txt", "r") as FILE:
    lines4 = [line[:-1] for line in FILE.readlines()]

rows1 = len(lines1)
rows2 = len(lines2)
rows3 = len(lines3)
rows4 = len(lines4)

cols = 80

with open("logo.h", "w") as FILE:
    FILE.write("#include \"types.h\"")
    FILE.write("\n\n")
    FILE.write(f"#define LOGO_ROWS1 {rows1}\n")
    FILE.write(f"#define LOGO_ROWS2 {rows2}\n")
    FILE.write(f"#define LOGO_ROWS3 {rows3}\n")
    FILE.write(f"#define LOGO_ROWS4 {rows4}\n")
    FILE.write(f"#define LOGO_ATTRIB 0x5F\n\n")
    FILE.write("int8_t logo_chars1["+ str(rows1) +"]["+ str(cols) +"] = {\n")
    
    for line in lines1:
        line_len = len(line)
        line_str = "\t"
        line_str += "{"
        for i in range(80):
            if i < line_len:
                if line[i] == "\\":
                    line_str += f"'{line[i]}{line[i]}',"
                elif line[i] == "'":
                    line_str += f"'\{line[i]}',"
                else:
                    line_str += f"'{line[i]}',"
            else:
                line_str += f"' ',"
        line_str = line_str[:-1] + "},\n"
        FILE.write(line_str)

    FILE.write("};\n\n")

    FILE.write("int8_t logo_chars2["+ str(rows2) +"]["+ str(cols) +"] = {\n")
    
    for line in lines2:
        line_len = len(line)
        line_str = "\t"
        line_str += "{"
        for i in range(80):
            if i < line_len:
                if line[i] == "\\":
                    line_str += f"'{line[i]}{line[i]}',"
                elif line[i] == "'":
                    line_str += f"'\{line[i]}',"
                else:
                    line_str += f"'{line[i]}',"
            else:
                line_str += f"' ',"
        line_str = line_str[:-1] + "},\n"
        FILE.write(line_str)

    FILE.write("};\n")

    FILE.write("int8_t logo_chars3["+ str(rows3) +"]["+ str(cols) +"] = {\n")
    
    for line in lines3:
        line_len = len(line)
        line_str = "\t"
        line_str += "{"
        for i in range(80):
            if i < line_len:
                if line[i] == "\\":
                    line_str += f"'{line[i]}{line[i]}',"
                elif line[i] == "'":
                    line_str += f"'\{line[i]}',"
                else:
                    line_str += f"'{line[i]}',"
            else:
                line_str += f"' ',"
        line_str = line_str[:-1] + "},\n"
        FILE.write(line_str)

    FILE.write("};\n")

    FILE.write("int8_t logo_chars4["+ str(rows4) +"]["+ str(cols) +"] = {\n")
    
    for line in lines4:
        line_len = len(line)
        line_str = "\t"
        line_str += "{"
        for i in range(80):
            if i < line_len:
                if line[i] == "\\":
                    line_str += f"'{line[i]}{line[i]}',"
                elif line[i] == "'":
                    line_str += f"'\{line[i]}',"
                else:
                    line_str += f"'{line[i]}',"
            else:
                line_str += f"' ',"
        line_str = line_str[:-1] + "},\n"
        FILE.write(line_str)

    FILE.write("};\n")

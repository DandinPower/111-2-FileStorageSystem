import sys
import json
import subprocess
import summary

W = '\033[0m'  # white (normal)
R = '\033[31m'  # red
G = '\033[32m'  # green


# print a lot of "-"
def print_dash():
    for i in range(20):
        print("-", end='')
    print()


# get output from command
def get_Output(command):
    return subprocess.run(command, shell=True, stdout=subprocess.PIPE).stdout.decode('utf-8')


def compare(stu_output, answer):
    alphanumeric = ""
    for character in stu_output:
        if character.isprintable() or character == '\n':
            alphanumeric += character

    same = True
    i = 0
    alph_split = alphanumeric.split("\n")
    answer_split = answer.split("\n")

    alph = []
    ans = []
    for line in alph_split:
        if line.rstrip():
            alph.append(line.rstrip())

    for line in answer_split:
        if line.rstrip():
            ans.append(line.rstrip())

    alph_split = alph
    answer_split = ans

    if (len(alph_split) != len(answer_split)):  # judge false when the lengths are different
        same = False
    else:
        while (i < len(alph_split)):
            if "Ticks" in alph_split[i] or "Ticks" in answer_split[i]:
                i += 1
                continue
            if alph_split[i] != answer_split[i]:
                same = False
                break
            i += 1

    if same == False:
        print_diff(stu_output, answer)
    return same


def print_diff(stu_output, answer):
    print_dash()
    print("Expected output:")
    print(answer)
    print_dash()
    print("Your output:")
    print(stu_output)
    print_dash()


def test_case(file_name):
    sum = summary.Summary()
    with open(file_name, "r") as f:
        json_object = json.load(f)
        for case in json_object:
            print("{}[ RUN      ]{}".format(G, W), case['case_name'])

            command = case['command']
            print(command)
            students_output = get_Output(command)
            if case['isfile']:
                answer = open(case['answer'], 'r').read()
            else:
                answer = case['answer']

            correct = compare(students_output, answer)

            if correct:
                print("{}[       OK ] {}".format(G, W), end='')
                sum.addScore_Pass(case['score'])
            else:
                print("{}[  FAILED  ] {}".format(R, W), end='')
                sum.addScore_Fail(case['score'], case['case_name'])
            print(case['case_name'])
            # print(output)

    sum.print_Summary()


def main():
    if len(sys.argv) <= 1:
        print("no test case file")
        return 0
    test_case(sys.argv[1])


if __name__ == "__main__":
    main()

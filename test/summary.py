class Summary:
    def __init__(self):
        self.right = 0          # right score
        self.total = 0          # total score
        self.pass_test = 0      # amount of test cases
        self.fail_list = []
        self.W = '\033[0m'      # white (normal)
        self.R = '\033[31m'     # red
        self.G = '\033[32m'     # green

    def addScore_Fail(self, score, failcase):
        self.total += score
        self.fail_list.append(failcase)

    def addScore_Pass(self, score):
        self.total += score
        self.right += score
        self.pass_test += 1

    def print_Fail(self):
        for case in self.fail_list:
            print("{}[  FAILED  ]{}".format(self.R, self.W), case)

    def print_Summary(self):
        print("{}[  PASSED  ]{}".format(self.G, self.W), self.pass_test, "test", end='')
        if self.pass_test > 1:
            print("s", end='')
        print(".")
        if len(self.fail_list) > 0:
            print("{}[  FAILED  ]{}".format(self.R, self.W), len(self.fail_list), "test", end='')
            if len(self.fail_list) > 1:  # s or not s
                print("s", end='')
            print(", listed below:")

            self.print_Fail()

        print("Points: ", self.right, "/", self.total)

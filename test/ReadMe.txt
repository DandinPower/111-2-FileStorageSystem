這個Script不含編譯，要在執行test_case.py之前先編譯好執行檔案

test_case.json 檔欄位說明:
1. case_name : test case 名稱，讓自己辨識用
2. command : 包含路徑的指令(不含參數)，參數必須寫在後面的args，例如 "../build.linux/nachos -e consoleIO_test1" 則這個欄位只有 "../build.linux/nachos"
3. isfile : 表示答案是否為檔案，如果是true，會將後面的answer視為檔案路徑去讀取檔案，如果為false，就會直接將answer視為value直接比對
4. args : 指令的參數，例如 "../build.linux/nachos -e consoleIO_test1"，那這個欄位就要填 ["-e", "consoleIO_test1"]
5. answer : 答案的檔案路徑或是要比對的字串，看剛剛的 isfile 是 true 還是 false
6. score : 這個test_case的分數，最後會加總所有test_case的分數並顯示 得分/總分

備註 : 準備檔案時只要將指令導出到檔案即可，例如 "../build.linux/nachos -e halt > halt123.txt"，然後 answer 輸入 "halt123.txt"

備註 : CentOS 7 預設似乎只有 python2，要先安裝 python3 才可以執行，我是參考這個連結 https://kirin.idv.tw/python-install-python3-in-centos7/

備註 : 可能可以考慮改成 "../build.linux/nachos -e consoleIO_test1 5" 然後只輸出 "5\n" 的類似程式，這樣就可以直接將answer改成 "5\n" , args 改成 ["-e", "consoleIO_test1", "5"]，而不用準備很多檔案

使用步驟:

請先將所需執行檔編譯好
並將 test_case.py 以及 summary.py 放在 code/test/ 裡面
準備好test_case.json ( 檔名不限，只要是 json 檔並按照上述格式寫即可，test_case.json 為範例)
以及答案的txt(如果有的話)，也放在 code/test/ 裡面

在 code/test/ 底下輸入 python3 test_case.py test_case.json 即可
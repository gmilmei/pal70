;;; Major mode for PAL

(defconst pal-builtins
  '("Atom" "Conc" "Cy" "Isboolean" "Isdummy" "Isfunction"
    "Isinteger" "Islabel" "Isnumber" "Isprogramclosure"
    "Isreal" "Isstring" "Istruthvalue" "Istuple" "ItoR"
    "Length" "Lookupinj" "Null" "Order" "Pr" "Print"
    "Readch" "RtoI" "Share" "Stem" "Stern" "Stoi" "StoR"
    "Swing" "Tuple" "Write" "aug" "dummy" "eq" "false"
    "ge" "gr" "jj" "le" "ls" "nil" "or" "not" "true"))

(defconst pal-keywords
  '("and" "def" "do" "fn" "goto" "if" "ifnot" "ifso"
    "in" "let" "ll" "rec" "res" "test" "val" "valof"
    "where" "while" "within"))
(defun pal-make-regex (names)
  (concat "\\<\\("
          (mapconcat (lambda (x) x) names "\\|")
          "\\)\\>"))

(defconst pal-font-lock-keywords
  (list
   (list (pal-make-regex pal-keywords)
         1 'font-lock-keyword-face)
   (list (pal-make-regex pal-builtins)
         1 'font-lock-builtin-face)))

(defconst pal-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry ?_ "_" st)
    (modify-syntax-entry ?+ "." st)
    (modify-syntax-entry ?- "." st)
    (modify-syntax-entry ?$ "." st)
    (modify-syntax-entry ?& "." st)
    (modify-syntax-entry ?* "\\" st)
    (modify-syntax-entry ?| "." st)
    (modify-syntax-entry ?% "." st)
    (modify-syntax-entry ?< "." st)
    (modify-syntax-entry ?> "." st)
    (modify-syntax-entry ?= "." st)
    (modify-syntax-entry ?/ ". 124b" st)
    (modify-syntax-entry ?\' "\"" st)
    (modify-syntax-entry ?\n "> b" st)
    st))

(defconst pal-mode-map
  (let ((map (make-sparse-keymap)))
    ;;(define-key map "\C-c\C-c" 'pal-check-syntax)
    map)
  "Keymap used in PAL mode.")

(define-derived-mode pal-mode prog-mode "PAL"
  "Major mode for editing PAL code.\\<pal-mode-map>"
  (setq-local require-final-newline mode-require-final-newline)
  (use-local-map pal-mode-map)
  ;(setq-local indent-line-function 'pal-indent-line)
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "\\(//+\\)\\s *")
  (setq-local comment-style 'indent)
  (setq-local indent-tabs-mode nil)
  (setq-local tab-width 4)
  (setq-local font-lock-defaults '(pal-font-lock-keywords))
  (set-syntax-table pal-mode-syntax-table))

(provide 'pal-mode)

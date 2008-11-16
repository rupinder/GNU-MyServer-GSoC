;; MyServer
;; Copyright (C) 2008 Free Software Foundation, Inc.
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

(require 'starttls)

;;   Basic usage for the library:
;;
;;(setq my-proc (myserver-control-init "localhost" 12345))
;;(myserver-control-send-message my-proc (concat "/SHOWCONNECTIONS CONTROL/1.0 \r\n"
;;                                               "/AUTH ADMIN:ADMIN\r\n"
;;                                               "/CONNECTION Keep-Alive\r\n\r\n"))
;;
;;(myserver-control-end my-proc)

(defun myserver-control-init (server port)
  "Initialize a connection to the specified SERVER using the PORT."
  (when (get-buffer "myserver") (kill-buffer "myserver"))
  (let ((proc (starttls-open-stream "myserver" 
                                       (get-buffer-create "myserver")
                                       server
                                       port)))
    (starttls-negotiate proc)
    proc))
    

(defun myserver-control-send-message (proc msg)
  "Send the message MSG to the specified PROC and
   return the response header lines."
  (process-send-string proc msg)
  
  (with-current-buffer "myserver"
    (let ((i 3) 
          (np 0)
          (start 0)
          (curr nil)
          (res '()))
      (while (and (> i 0)
                  (not (search-forward "\r\n\r\n" nil t)))
        (myserver-control-wait-for-data my-proc 2)
        (setq i (1- i))
        (goto-char (point-min))  )
      
      (goto-char (point-min))
      (setq res (append res (list (buffer-substring 2 (- (search-forward "\r\n" nil t) 2)))))
      
      (while (progn
               (setq start (1+ (point)))
               (setq np (- (search-forward "\r" nil t) 1))
               (setq curr (buffer-substring start np))
               (> (- np start) 4))
        
        (setq res (append res (list curr)))) 
      res
      )))

(defun myserver-control-wait-for-data (proc sec)
  "Read data from the process using a timeout of SEC seconds."
  (accept-process-output proc sec 0))

(defun myserver-control-end (proc)
  "Close the connection to the server."
  (delete-process proc))


(provide 'myserver-control)

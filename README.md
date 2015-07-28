# nohup-email

A modified nohup which notifies you via email when the process has completed.

======

The traditional nohup command works by making the process ignore the SIGHUP
signal from the parent process (usually the terminal). Its mostly used when
running long jobs over a SSH session which doesn't need interaction and should
continue even when the session ends. I face this a lot while running my machine
learning projects, hence I combined nohup and cURL to notify me via email.

A small modification in nohup-email from the traditional nohup is that the
latter runs the given command in the same process and hence there is no way to
know when its finished. nohup-email instead forks off the child process and
waits for it to finish after which the email is sent.

The only configuration reqiured is to have a small `.nohup` file in your home
folder with the following format:

```
fromemail=<FromEmail@example.com>
toemail=<ToEmail@example.com>
password=<password>
```

The utility assumes that the **username for login is the first half of the from
email address**.

**Currently you will need to store the password for the email in plain text and
hence I would recommend this account be not your primary email**.

**The utility currently has Yahoo SMTP hard coded**.

I am yet to find a way to make it work with Gmail without registering the app
with Google Apps.

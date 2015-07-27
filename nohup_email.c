/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2014, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "nohup_email.h"

struct upload_status {
  int lines_read;
  char **payload;
};

struct nohup_email{
  char *from_email;
  char *to_email;
  char *username;
  char *passwd;
  char *process_name;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  char **payload_text = upload_ctx->payload;
  const char *data;

  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }

  data = payload_text[upload_ctx->lines_read];
  
  if(data) {
    size_t len = strlen(data);
    memcpy(ptr, data, len);
    upload_ctx->lines_read++;

    return len;
  }

  return 0;
}

int nohup_email_helper(struct nohup_email settings)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;

  upload_ctx.lines_read = 0;
  
  char subject[150],date[100],to[1000],from[100];

  time_t now = time(NULL);
  sprintf(date,"Date: %s +0530\r\n",ctime(&now)); 
  sprintf(to,"To: %s\r\n",settings.to_email);
  sprintf(from,"From: %s\r\n",settings.from_email);
  sprintf(subject,"Subject: %s has finished\r\n",settings.process_name);
  
  char *payload_text[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    "\r\n", /* empty line to divide headers from body, see RFC5322 */
    "This email is to inform that the process mentioned\r\n",
    "in the subject has completed. Please login and\r\n",
    "check the nohup file as well.\r\n",
    NULL
  };
  
  payload_text[0] = date;
  payload_text[1] = to;
  payload_text[2] = from;
  payload_text[3] = subject;
  upload_ctx.payload = payload_text;
  
  curl = curl_easy_init();
  if(curl) {
    /* Set username and password */
    curl_easy_setopt(curl, CURLOPT_USERNAME, settings.username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, settings.passwd);

    /* This is the URL for your mailserver. Note the use of port 587 here,
     * instead of the normal SMTP port (25). Port 587 is commonly used for
     * secure mail submission (see RFC4403), but you should use whatever
     * matches your server configuration. */
    
    /* Need to generalize this based on the server */
    curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.mail.yahoo.com:587");

    /* In this example, we'll start with a plain text connection, and upgrade
     * to Transport Layer Security (TLS) using the STARTTLS command. Be careful
     * of using CURLUSESSL_TRY here, because if TLS upgrade fails, the transfer
     * will continue anyway - see the security discussion in the libcurl
     * tutorial for more details. */
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

    /* If your server doesn't have a valid certificate, then you can disable
     * part of the Transport Layer Security protection by setting the
     * CURLOPT_SSL_VERIFYPEER and CURLOPT_SSL_VERIFYHOST options to 0 (false).
     *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
     *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
     * That is, in general, a bad idea. It is still better than sending your
     * authentication details in plain text though.
     * Instead, you should get the issuer certificate (or the host certificate
     * if the certificate is self-signed) and add it to the set of certificates
     * that are known to libcurl using CURLOPT_CAINFO and/or CURLOPT_CAPATH. See
     * docs/SSLCERTS for more information. */
    //curl_easy_setopt(curl, CURLOPT_CAINFO, "");

    /* Note that this option isn't strictly required, omitting it will result in
     * libcurl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise, they
     * could cause an endless loop. See RFC 5321 Section 4.5.5 for more details.
     */
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, settings.from_email);

    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */
    recipients = curl_slist_append(recipients, settings.to_email);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcurl to see what is happening during the transfer.
     */
    long debug = -1;
#ifdef DEBUG
    debug = 1L;
#else
    debug = 0L;
#endif
    curl_easy_setopt(curl, CURLOPT_VERBOSE, debug);

    /* Send the message */
    res = curl_easy_perform(curl);

    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* Free the list of recipients */
    curl_slist_free_all(recipients);

    /* Always cleanup */
    curl_easy_cleanup(curl);
  }

  return (int)res;
}

void nohup_email_cleanup(struct nohup_email settings){
  if(settings.to_email!=NULL)
    free(settings.to_email);
  if(settings.from_email!=NULL)
    free(settings.from_email);
  if(settings.username!=NULL)
    free(settings.username);
  if(settings.passwd!=NULL)
    free(settings.passwd);
}

int nohup_email_send(char *process_name){
  char *home = getenv("HOME");
  char settings_file[100];
  sprintf(settings_file,"%s/.nohup",home);
  
  FILE *fp = fopen(settings_file,"r");
  if(fp==NULL){
    fprintf(stderr,"Settings file not found\n");
    return -1;
  }
  struct nohup_email settings;
  settings.from_email = (char*)malloc(100);
  settings.to_email = (char*)malloc(100);
  settings.username = (char*)malloc(100);
  settings.passwd = (char*)malloc(100);
  settings.process_name = process_name;
  
  char *line = (char*)malloc(100);
  char *tok;
  size_t line_size = 100;

  /* get the from email */
  getline(&line,&line_size,fp);
  tok = strtok(line,"=\n");
  tok = strtok(NULL,"=\n");
#ifdef DEBUG
  printf("From Email %s\n",tok);
#endif
  strcpy(settings.from_email,tok);

  /* get the to email */
  getline(&line,&line_size,fp);
  tok = strtok(line,"=\n");
  tok = strtok(NULL,"=\n");
#ifdef DEBUG
  printf("To Email %s\n",tok);
#endif
  strcpy(settings.to_email,tok);

  /* get the password */
  getline(&line,&line_size,fp);
  tok = strtok(line,"=\n");
  tok = strtok(NULL,"=\n");
#ifdef DEBUG
  printf("Password %s\n",tok);
#endif
  strcpy(settings.passwd,tok);

  /* get username from, fromemail */
  char *from_email_duplicate = (char*) malloc(100);
  strcpy(from_email_duplicate,settings.from_email);
  tok = strtok(from_email_duplicate,"@\n");
  strcpy(settings.username,tok);
  free(from_email_duplicate);
#ifdef DEBUG
  printf("Username %s\n",settings.username);
#endif
  free(line);
  int ret = nohup_email_helper(settings);
  nohup_email_cleanup(settings);
  return ret;
}

/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-23 I/O Performance, Inc.
 * Copyright (C) 2009-23 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
# --target # <in|out|int> #WorkerThreads     <file|net|sg> <options>
# --target # <in|out|int> #WorkerThreads     file          <filename>      <file_size>          <transfer_size>  <dio,null,sync>
# --target # <in|out|int> #WorkerThreads     network       <client|server> <user@hostname:port> <TCP|UDP|UDT>
# --target # <in|out|int> #WorkerThreads     sg            /dev/sg#
./bxt --target 0 in  3 file /tmp/test_input_file  12345678 1048576 bio \
      --target 1 out 1 file /tmp/test_output_file 12345678 1048576 bio \
      --target 2 out 2 file /tmp/test_output_file2 1234567  1048576 bio \
	  --buffers 3 \
	  --sequence 0 1 2

  



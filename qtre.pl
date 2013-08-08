# qtre.pl
# Global macros and initializations for qtre.
# Revision 2-mar-2006

package qtre;

use strict;

#*********************************************************************
#			Create menu options
#*********************************************************************


menu_add_option 'File', -1, 'open &Url', 'open_url';


#*********************************************************************
#			Translation
#*********************************************************************

{
	my $lang= substr ($ENV{LANG}, 0, 2);

	$lang='es' if ($lang == '');

	if ($lang eq 'es')
	{

#	Standard menu options.

menu_rename 'File/Open',                    '&Abrir';
menu_rename 'File/New',                     '&Nuevo';
menu_rename 'File/Save',                    '&Guardar';
menu_rename 'File/save As',                 'gu&Ardar como';
menu_rename 'File/nexT',                    '&Siguiente';
menu_rename 'File/Close',                   '&Cerrar';
menu_rename 'File/eXit',                    '&Terminar';
menu_rename 'File/sElect',                  's&Eleccionar';
menu_rename 'File/opTions',                 '&Opciones';

#	Defined in this file.
menu_rename 'File/open Url',                'abrir &Url';

menu_rename 'Edit/Select',                  '&Seleccionar';
menu_rename 'Edit/cuT',                     'cor&Tar';
menu_rename 'Edit/Copy',                    '&Copiar';
menu_rename 'Edit/Paste',                   '&Pegar';
menu_rename 'Edit/Delete',                  'b&Orrar';
menu_rename 'Edit/select &All',             'sel. &Todo';
menu_rename 'Edit/copy to New',             'copiar a &Nuevo';
menu_rename 'Edit/Options',                 'opc&Iones';

menu_rename 'Find/Find',                    '&Buscar';
menu_rename 'Find/find Again',              'buscar de &Nuevo';
menu_rename 'Find/Replace',                 '&Reemplazar';

menu_rename 'Go/Begin',                     '&Inicio';
menu_rename 'Go/End',                       '&Fin';
menu_rename 'Go/line Number',               'linea &Numero';

# System/Shell not translated.
menu_rename 'System/Execute',               '&Ejecutar';
menu_rename 'System/execute Line',          'ejecutar &Linea';
# System/Prompt not translated.

menu_rename 'Perl/Evaluate',                '&Evaluar';
menu_rename 'Perl/evaluate Selection',      'evaluar &Seleccion';
menu_rename 'Perl/evaluate Clipboard',      'evaluar &Portapapeles';
menu_rename 'Perl/evaluate Line',           'evaluar &Linea';
# Perl/Define control not translated.

menu_rename 'pRogramming/Compile',          '&Compilar';
# pRogramming/Make not translated.
menu_rename 'pRogramming/Options',          '&Opciones';
# pRogramming/mAn not translated.
menu_rename 'pRogramming/man oF',           'man &De';

menu_rename 'Help/Index',                   '&Indice';
menu_rename 'Help/Perl functions',          'funciones &Perl';
menu_rename 'Help/About',                   '&Acerca de';

# Main menu bar.

menu_rename 'File',                         '&Archivo';
menu_rename 'Edit',                         '&Edicion';
menu_rename 'Find',                         '&Buscar';
menu_rename 'Go',                           '&Ir';
menu_rename 'System',                       '&Sistema';
# Perl not translated.
menu_rename 'pRogramming',                  'p&Rogramacion';
menu_rename 'Help',                         'a&Yuda';

	}
}

#*********************************************************************
#	Define keys.
#*********************************************************************

#define_control 'C', 'copy';
#define_control 'H', 'backspace';
#define_control 'L', 'redraw';
#define_control 'Q', 'quit';
#define_control 'S', 'toggle_select';
define_control 'T', 'insert_special_char';
#define_control 'V', 'paste';
#define_control 'X', 'cut';
#define_control 'Y', 'delete_line';

sub insert_special_char ()
{
	my $c= get_key;
	$c= 13 if ($c == 10);
	insert_char $c if ($c >= 0 && $c < 256);
}

#*********************************************************************
#	Redirect perl standard output to text insertion.
#*********************************************************************

{
	package QtreRedirectOutput;

	sub TIEHANDLE { bless \( my $scalar ), shift; }
	sub PRINT
	{
		my $self= shift;
		qtre::insert (join "", @_);
	}
	sub PRINTF
	{
		my $self= shift;
		my $fmt= shift;
		qtre::insert (sprintf $fmt, @_);
	}
}

tie ( *QTREOUTPUT, 'QtreRedirectOutput') or die "Failed!\n";

select QTREOUTPUT;

#*********************************************************************
#	Auxiliary functions.
#*********************************************************************

#
# Returns current date in short format.
#
sub date
{
	my ($day, $mon, $year)= (localtime) [3, 4, 5];
	$year+= 1900;
	my $m= ('jan', 'feb', 'mar', 'apr', 'may', 'jun',
		'jul', 'aug', 'sep', 'oct', 'nov', 'dec') [$mon];
	$day . '-' . $m . '-' . $year;
}

#
# Find a date stamp in current file and prompt for update it.
#
sub update_date ()
{
	my $line= line;
	my $pos= get_pos;
	my $save_it= 1;

	#goto_line 0;
	#home;
	#quiet 1;
	my $datef=
		'(\d{1,2}-(jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)' .
		'-\d{2,4})';
	my $actual= 0;
	my $found= 0;
	my $endline= num_lines;
	$endline= 30 if $endline > 30;
	for (;;)
	{
		if (get_line ($actual) =~ /$datef/ )
		{
			$found= 1;
			#right $-[1];
			my $posdate= $-[1];
			#update;
			my $current= date;
			status $current;
			if (! ($current eq $1) )
			{
				#update;
				quiet 1;
				goto_line $actual;
				home;
				right $posdate;
				update;
				my $r= status_question
					'Update date (Yes,No, Cancel?',
					'YyNnCc';
				if ($r eq 'Y' || $r eq 'y')
				{
					my $l= get_line;
					$l= substr ($l, 0, $-[1]) . $current .
						substr ($l, $+[1]);
					replace_line $l;
				}
				if ($r eq 'C' || $r eq 'c')
				{
					#status "Cancelled";
					#return 0;
					$save_it= 0;
					last;
				}
				status "";
			}
			last;
		}
		++$actual;
		last if $actual >= $endline;
	}
	if ($found)
	{
		goto_line $line;
		home;
		right while ($pos-- > 0);
		quiet 0;
		update;
	}
	return $save_it;
}

#
# Open a url and insert the content in a new document.
#
sub open_url ()
{
	eval 'local $SIG{"__DIE__"}; require LWP::Simple;';

	die 'Module LWP::Simple not available' if $@;

	my $url= dialog_get_string 'Open URL', 'Write the URL to open',
		60, 255;
	return unless defined $url;
	status "Loading URL '$url'...";
	my $content= LWP::Simple::get $url;
	if (! defined $content)
	{
		status "'$url' not found";
		return;
	}
	file_new;
	status $url;
	insert $content;
}

#*********************************************************************
#	Events and menu options.
#*********************************************************************

#
# After creating a new C or C++ file, put a header and a footer in it.
#
sub new_file
{
	my $ext= get_fileext;
	my $name= get_filename;
	if ($ext eq 'cpp' || $ext eq 'h')
	{
		my $iniline= 3;
		if ($ext eq 'h')
		{
			my $guard= $name;
			$guard=~ tr/[a-z]\./[A-Z]_/;
			$guard= "INCLUDE_$guard";
			insert "#ifndef $guard\n#define $guard\n\n";
			$iniline+= 3;
		}
		insert "// " . $name . "\n// Revision " . date . "\n\n\n\n";
		insert "#endif\n" if ($ext eq 'h');
		insert "\n// End of $name\n";
		goto_line $iniline;
	}
	if ($ext eq "c")
	{
		insert ("/*\n\t$name \n\tRevision " . date .
			"\n*/\n\n\n\n/* End of $name */\n" );
		goto_line 5;
	}
	modified 0;
}

#
# Before saving the file, prompt for update revision date.
#
sub file_save
{
	qtre::default::file_save if update_date;
}

#*********************************************************************
#	Macros
#*********************************************************************

#
#	Replace space indenting with tabs in the selection.
#
sub macro_ctrl_i
{
	my ($begline, $begpos)= get_beginsel;
	return unless defined $begline;
	my ($endline, $endpos)= get_endsel;
	++$begline if $begpos;
	--$endline if ! $endpos;
	my $count= 0;
	my $i;
	for ($i= $begline; $i <= $endline; ++$i)
	{
		#status $i;
		my $l= get_line $i;
		my $n= 0;
		my $pos= 0;
		while (substr ($l, $pos, 8) eq '        ')
			{ ++$n; $pos+= 8; }
		if ($n > 0)
		{
			$l= "\t" x $n . substr ($l, $pos);
			replace_line ($l, $i);
			++ $count;
		}
	}
	status 'Lines tabbed: ' . $count;
}

#
# Find the matching character.
#
sub macro_28 # Macro (
{
	my $c= get_char ();
	$_= $c;
	my $forward= /[\(\[{"']/;
	if (! tr/([{)]}"'/)]}([{"'/ )
	{
		status "Character not valid";
		return;
	}
	my $s= $_;
	$c= "" if ($c eq '"' || $c eq "'");
	my $endline= num_lines () - 1;
	my $deep= 1;
	my $nline= line ();
	my $l= char_line ();
	my $current= get_line ();
	my $pos= get_pos ();
	mainloop: while (1)
	{
		if ($forward)
		{
			++$pos;
			if ($pos >= $l)
			{
				last mainloop if $nline == $endline;
				++$nline;
				$current= get_line ($nline);
				$l= char_line ($nline);
				next mainloop if $l == 0;
				$pos= 0;
			}
		}
		else
		{
			if ($pos == 0)
			{
				last mainloop if $nline == 0;
				--$nline;
				$current= get_line ($nline);
				$l= char_line ($nline);
				next mainloop if $l == 0;
				$pos= $l - 1;
			}
			else { --$pos; }
		}

		# my $actual= get_char ();
		my $actual= substr ($current, $pos, 1);

		if ($actual eq $c) { ++$deep; }
		else
		{
			if ($actual eq $s)
			{
			      --$deep;
			      last mainloop if ($deep == 0);
			}
		}
	}
	if ($deep == 0)
	{
		goto_line $nline;
		home;
		my $n;
		for ($n= 0; $n <  $pos; ++$n) { right; }
	}
	else { status "Match not found"; }
}

#
#	Comment selection
#
sub macro_c
{
	my ($begline, $begpos)= get_beginsel;
	return unless defined $begline;
	end_select;
	my ($endline, $endpos)= get_endsel;
	++$begline if $begpos;
	--$endline if ! $endpos;
	my $ext= get_fileext;
	my $comment;
	if ($ext eq 'cpp' || $ext eq 'h')
		{ $comment= '//'; }
	else
		{ $comment= '#'; }

	my $i;
	for ($i= $begline; $i <= $endline; ++$i)
	{
		replace_line $comment . get_line ($i), $i
	}
}

#
#	Indent selection
#
sub macro_i
{
	my ($begline, $begpos)= get_beginsel;
	return unless defined $begline;
	end_select;
	my ($endline, $endpos)= get_endsel;
	++$begline if $begpos;
	--$endline if ! $endpos;
	my $i;
	my $tab= "\t";
	my $line;
	for ($i= $begline; $i <= $endline; ++$i)
	{
		# Do not touch empty lines.
		$line= get_line ($i);
		replace_line ($tab . $line, $i) if (length ($line) > 0);
	}
	status "Indent finished";
}

#
#	Deletes the ^M at end of line of ms-dos files
#
sub macro_m ()
{
	my $fin= num_lines ();
	my $i= 0;
	my $t;
	while ($i < $ fin)
	{
		$t= get_line $i;
		if (substr ($t, length ($t) - 1, 1) eq "\r")
		{
			chop ($t);
			replace_line $t, $i;
		}
		++$i;
	}
}

#
#	Append a ^M at the end of lines to convert to ms-dos txt format
#
sub macro_n ()
{
	my $fin= num_lines ();
	my $i= 0;
	my $t;
	while ($i < $ fin)
	{
		$t= get_line $i;
		if (substr ($t, length ($t) - 1, 1) ne "\r")
		{
			$t= $t . "\r";
			replace_line $t, $i;
		}
		++$i;
	}
}

#
#	Uncomment selection
#
sub macro_t
{
	my ($begline, $begpos)= get_beginsel;
	return unless defined $begline;
	end_select;
	my ($endline, $endpos)= get_endsel;
	++$begline if $begpos;
	--$endline if ! $endpos;
	my $ext= get_fileext;
	my $comment;
	if ($ext eq 'cpp' || $ext eq 'h')
		{ $comment= '//'; }
	else
		{ $comment= '#'; }
	my $lcom= length $comment;

	my $i;
	for ($i= $begline; $i <= $endline; ++$i)
	{
		my $l= get_line $i;
		if (substr ($l, 0, $lcom) eq $comment)
		{
			replace_line substr ($l, $lcom), $i
		}
	}
}

#
#	Unindent selection
#
sub macro_u
{
	my ($begline, $begpos)= get_beginsel;
	return unless defined $begline;
	end_select;
	my ($endline, $endpos)= get_endsel;
	++$begline if $begpos;
	--$endline if ! $endpos;
	my $i;
	my $tab= "\t";
	my $spaces8= ' ' x 8;
	for ($i= $begline; $i <= $endline; ++$i)
	{
		my $l= get_line $i;
		if (substr ($l, 0, 1) eq $tab)
		{
			replace_line substr ($l, 1), $i;
		}
		else
		{
			if (substr ($l, 0, 8) eq $spaces8)
			{
				replace_line substr ($l, 8), $i;
			}
		}
	}
}

#*********************************************************************

1;

# End of qtre.pl

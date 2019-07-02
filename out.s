insert:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$insert.size, %eax
	subq	%rax, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp),%r11
	cmpq		$0, %r11
	sete	%r11b
	movzbl	%r11b, %r11d
	cmpl	$0, %r11d
	je	.L0
	movq	$8,%r11
	imulq	$3, %r11
	movq	%r11,%rdi
	movl	$0, %eax
	call	malloc
	movq	%rax, -8(%rbp)
	movq	-8(%rbp),%r11
	addq	$0, %r11
	movq	-16(%rbp),%r10
	movq	%r10, (%r11)
	movq	-8(%rbp),%r11
	addq	$8, %r11
	movq	null,%r10
	movq	%r10, (%r11)
	movq	-8(%rbp),%r11
	addq	$16, %r11
	movq	null,%r10
	movq	%r10, (%r11)
	jmp	.L1
.L0:
	movq	-8(%rbp),%r11
	addq	$0, %r11
	movq		(%r11), %r11
	movq	-16(%rbp),%r10
	cmpq	%r11, %r10
	setl	%r10b
	movzbl	%r10b, %r10d
	cmpl	$0, %r10d
	je	.L2
	movq	-16(%rbp),%rsi
	movq	-8(%rbp),%r11
	addq	$8, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	call	insert
	movq	-8(%rbp),%r11
	addq	$8, %r11
	movq	%rax, (%r11)
	jmp	.L3
.L2:
	movq	-8(%rbp),%r11
	addq	$0, %r11
	movq		(%r11), %r11
	movq	-16(%rbp),%r10
	cmpq	%r11, %r10
	setg	%r10b
	movzbl	%r10b, %r10d
	cmpl	$0, %r10d
	je	.L4
	movq	-16(%rbp),%rsi
	movq	-8(%rbp),%r11
	addq	$16, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	call	insert
	movq	-8(%rbp),%r11
	addq	$16, %r11
	movq	%rax, (%r11)
.L4:
.L3:
.L1:
	movq	-8(%rbp),%rax
	jmp	insert.exit

insert.exit:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	insert.size, 16
	.globl	insert

search:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$search.size, %eax
	subq	%rax, %rsp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-8(%rbp),%r11
	cmpq		$0, %r11
	sete	%r11b
	movzbl	%r11b, %r11d
	cmpl	$0, %r11d
	je	.L6
	movq	%rax,-24(%rbp)
	movl	$0,%eax
	jmp	search.exit
.L6:
	movq	-8(%rbp),%r11
	addq	$0, %r11
	movq		(%r11), %r11
	movq	-16(%rbp),%r10
	cmpq	%r11, %r10
	setl	%r10b
	movzbl	%r10b, %r10d
	cmpl	$0, %r10d
	je	.L8
	movq	-16(%rbp),%rsi
	movq	-8(%rbp),%r11
	addq	$8, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	movl	%eax,-28(%rbp)
	call	search
	jmp	search.exit
.L8:
	movq	-8(%rbp),%r11
	addq	$0, %r11
	movq		(%r11), %r11
	movq	-16(%rbp),%r10
	cmpq	%r11, %r10
	setg	%r10b
	movzbl	%r10b, %r10d
	cmpl	$0, %r10d
	je	.L10
	movq	-16(%rbp),%rsi
	movq	-8(%rbp),%r11
	addq	$16, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	movl	%eax,-32(%rbp)
	call	search
	jmp	search.exit
.L10:
	movl	%eax,-36(%rbp)
	movl	$1,%eax
	jmp	search.exit

search.exit:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	search.size, 48
	.globl	search

preorder:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$preorder.size, %eax
	subq	%rax, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp),%r11
	cmpq	$0, %r11
	je	.L12
	movq	-8(%rbp),%r11
	addq	$0, %r11
	movq		(%r11), %r11
	movl		(%r11), %r11d
	movl	%r11d,%esi
	leaq	.L14, %r11
	movq	%r11,%rdi
	movl	%eax,-12(%rbp)
	movl	$0, %eax
	call	printf
	movq	-8(%rbp),%r11
	addq	$8, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	call	preorder
	movq	-8(%rbp),%r11
	addq	$16, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	call	preorder
.L12:

preorder.exit:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	preorder.size, 16
	.globl	preorder

inorder:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$inorder.size, %eax
	subq	%rax, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp),%r11
	cmpq	$0, %r11
	je	.L16
	movq	-8(%rbp),%r11
	addq	$8, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	call	inorder
	movq	-8(%rbp),%r11
	addq	$0, %r11
	movq		(%r11), %r11
	movl		(%r11), %r11d
	movl	%r11d,%esi
	leaq	.L14, %r11
	movq	%r11,%rdi
	movl	$0, %eax
	call	printf
	movq	-8(%rbp),%r11
	addq	$16, %r11
	movq		(%r11), %r11
	movq	%r11,%rdi
	call	inorder
.L16:

inorder.exit:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	inorder.size, 16
	.globl	inorder

main:
	pushq	%rbp
	movq	%rsp, %rbp
	movl	$main.size, %eax
	subq	%rax, %rsp
	movl	$0,%r11d
	movl	%r11d, -52(%rbp)
.L18:
	movl	-52(%rbp),%r11d
	cmpl	$8, %r11d
	setl	%r11b
	movzbl	%r11b, %r11d
	cmpl	$0, %r11d
	je	.L19
	leaq	-48(%rbp), %r11
	movslq	-52(%rbp), %r10
	imulq	$4, %r10
	addq	%r10, %r11
	movl	-52(%rbp),%r10d
	movl	%r10d, (%r11)
	movl	-52(%rbp),%r11d
	addl	$1, %r11d
	movl	%r11d, -52(%rbp)
	jmp	.L18
.L19:
	movq	null,%r11
	movq	%r11, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$28, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$16, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$4, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$0, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$20, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$8, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$12, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	-48(%rbp), %r11
	addq	$24, %r11
	movq	%r11,%r10
	movq	%r10,%rsi
	movq	-8(%rbp),%rdi
	call	insert
	movq	%rax, -8(%rbp)
	leaq	.L20, %r11
	movq	%r11,%rdi
	movl	$0, %eax
	call	printf
	movq	-8(%rbp),%rdi
	call	preorder
	leaq	.L22, %r11
	movq	%r11,%rdi
	movl	$0, %eax
	call	printf
	movq	-8(%rbp),%rdi
	call	inorder

main.exit:
	movq	%rbp, %rsp
	popq	%rbp
	ret

	.set	main.size, 64
	.globl	main

	.comm	null, 8
.L14:	.asciz	"%d\n"
.L22:	.asciz	"inorder traversal:\n"
.L20:	.asciz	"preorder traversal:\n"

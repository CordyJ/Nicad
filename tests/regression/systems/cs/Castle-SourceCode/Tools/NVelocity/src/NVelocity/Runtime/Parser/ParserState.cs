/* Generated By:JJTree: Do not edit this line. JJTParserState.java */

namespace NVelocity.Runtime.Parser
{
	using System;
	using System.Collections;
	using Node;

	internal class ParserState
	{
		private Stack nodes;
		private Stack marks;

		private int sp; // number of nodes on stack
		private int mk; // current mark
		private bool node_created;

		internal ParserState()
		{
			nodes = new Stack();
			marks = new Stack();
			sp = 0;
			mk = 0;
		}

		/* Determines whether the current node was actually closed and
	pushed.  This should only be called in the final user action of a
	node scope.  */

		internal bool NodeCreated()
		{
			return node_created;
		}

		/* Call this to reinitialize the node stack.  It is called
	automatically by the parser's ReInit() method. */

		internal void Reset()
		{
			nodes.Clear();
			marks.Clear();
			sp = 0;
			mk = 0;
		}

		/* Returns the root node of the AST.  It only makes sense to call
	this after a successful parse. */

		internal INode RootNode
		{
			get { return (INode) (nodes.ToArray())[nodes.Count - (0 + 1)]; }
		}

		/* Pushes a node on to the stack. */

		internal void PushNode(INode n)
		{
			Object temp_object;
			temp_object = n;
			Object generatedAux = temp_object;
			nodes.Push(temp_object);
			++sp;
		}

		/* Returns the node on the top of the stack, and remove it from the
	stack.  */

		internal INode PopNode()
		{
			if (--sp < mk)
			{
				mk = ((Int32) marks.Pop());
			}
			return (INode) nodes.Pop();
		}

		/* Returns the node currently on the top of the stack. */

		internal INode PeekNode()
		{
			return (INode) nodes.Peek();
		}

		/* Returns the number of children on the stack in the current node
	scope. */

		internal int NodeArity()
		{
			return sp - mk;
		}


		internal void ClearNodeScope(INode n)
		{
			while(sp > mk)
			{
				PopNode();
			}
			mk = ((Int32) marks.Pop());
		}


		internal void OpenNodeScope(INode n)
		{
			Object temp_object;
			temp_object = mk;
			Object generatedAux = temp_object;
			marks.Push(temp_object);
			mk = sp;
			n.Open();
		}


		/* A definite node is constructed from a specified number of
	children.  That number of nodes are popped from the stack and
	made the children of the definite node.  Then the definite node
	is pushed on to the stack. */

		internal void CloseNodeScope(INode n, int num)
		{
			mk = ((Int32) marks.Pop());
			while(num-- > 0)
			{
				INode c = PopNode();
				c.Parent = n;
				n.AddChild(c, num);
			}
			n.Close();
			PushNode(n);
			node_created = true;
		}


		/* A conditional node is constructed if its condition is true.  All
	the nodes that have been pushed since the node was opened are
	made children of the the conditional node, which is then pushed
	on to the stack.  If the condition is false the node is not
	constructed and they are left on the stack. */

		internal void CloseNodeScope(INode n, bool condition)
		{
			if (condition)
			{
				int a = NodeArity();
				mk = ((Int32) marks.Pop());
				while(a-- > 0)
				{
					INode c = PopNode();
					c.Parent = n;
					n.AddChild(c, a);
				}
				n.Close();
				PushNode(n);
				node_created = true;
			}
			else
			{
				mk = ((Int32) marks.Pop());
				node_created = false;
			}
		}
	}
}
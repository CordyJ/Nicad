// Copyright 2004-2007 Castle Project - http://www.castleproject.org/
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

namespace Castle.DynamicProxy.Builder.CodeBuilder.SimpleAST
{
	using System;
	using System.Reflection.Emit;
	using Castle.DynamicProxy.Builder.CodeBuilder.Utils;

	/// <summary>
	/// Wraps a reference that is passed ByRef and provides indirect load/store facilities.
	/// </summary>
	[CLSCompliant(false)]
	public class IndirectReference : TypeReference
	{
		public IndirectReference(TypeReference byRefReference) : base(byRefReference, byRefReference.Type.GetElementType())
		{
			if (! byRefReference.Type.IsByRef)
				throw new ArgumentException("Expected a reference whose type IsByRef", "byRefReference");
		}

		public static TypeReference WrapIfByRef(TypeReference reference)
		{
			return reference.Type.IsByRef ? new IndirectReference(reference) : reference;
		}

		public static TypeReference[] WrapIfByRef(TypeReference[] references)
		{
			TypeReference[] result = new TypeReference[references.Length];

			for (int i = 0; i < references.Length; i++)
			{
				result[i] = WrapIfByRef(references[i]);
			}

			return result;
		}

		public override void LoadReference(ILGenerator gen)
		{
			OpCodeUtil.EmitLoadIndirectOpCodeForType(gen, Type);
		}

		public override void StoreReference(ILGenerator gen)
		{
			OpCodeUtil.EmitStoreIndirectOpCodeForType(gen, Type);
		}

		public override void LoadAddressOfReference(ILGenerator gen)
		{
			// Load of owner reference takes care of this.
		}
	}
}
